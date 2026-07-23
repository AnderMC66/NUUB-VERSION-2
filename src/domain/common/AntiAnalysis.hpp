#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <intrin.h>
#include <cstring>

#pragma comment(lib, "Psapi.lib")

namespace nuub::domain::anti {

class AntiDebug {
public:
    static bool is_debugger_present() {
        return IsDebuggerPresent() != 0;
    }

    static bool check_peb_debugger() {
        #ifdef _WIN64
        auto* peb = reinterpret_cast<uintptr_t*>(__readgsqword(0x60));
        #else
        auto* peb = reinterpret_cast<uintptr_t*>(__readfsdword(0x30));
        #endif
        return *reinterpret_cast<uint8_t*>(peb + 2) != 0;
    }

    static bool check_nt_global_flag() {
        #ifdef _WIN64
        auto* peb = reinterpret_cast<uint8_t*>(__readgsqword(0x60));
        uint32_t offset = 0xBC;
        #else
        auto* peb = reinterpret_cast<uint8_t*>(__readfsdword(0x30));
        uint32_t offset = 0x68;
        #endif
        uint32_t nt_global_flag = *reinterpret_cast<uint32_t*>(peb + offset);
        return (nt_global_flag & 0x70) != 0;
    }

    static bool check_timing() {
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);

        volatile int x = 0;
        for (int i = 0; i < 1000; ++i) x += i;

        QueryPerformanceCounter(&end);

        double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
        return elapsed > 0.01;
    }

    static bool check_hardware_breakpoints() {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            return ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0;
        }
        return false;
    }

    // Enumerate processes using EnumProcesses (avoids tlhelp32.h /Za issues)
    static bool check_analysis_tools() {
        const char* tools[] = {
            "ollydbg.exe", "x64dbg.exe", "x32dbg.exe",
            "ida.exe", "ida64.exe", "idag.exe",
            "wireshark.exe", "fiddler.exe",
            "procmon.exe", "procmon64.exe",
            "processhacker.exe", "processhacker.bin",
            "cheatengine.exe", "cheatengine-i386.exe",
            "dnspy.exe", "de4dot.exe",
            "charles.exe", "tcpview.exe",
            "autoruns.exe", "regshot.exe",
            "apimonitor.exe", "binary.ninja",
            "ghidra.exe", "ghidraRun.exe",
            "radare2.exe", "r2.exe",
            "httpdebugger.exe", "httpdebuggerpro.exe",
            "mitmproxy.exe", "dumpcap.exe",
            "windbg.exe", "cdb.exe", "ntsd.exe",
            "dbgview.exe", "spyxx.exe",
            nullptr
        };

        DWORD processes[1024]{};
        DWORD cb_needed = 0;
        if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) return false;

        DWORD num_processes = cb_needed / sizeof(DWORD);
        bool found = false;

        for (DWORD i = 0; i < num_processes && !found; ++i) {
            if (processes[i] == 0) continue;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
            if (!hProcess) continue;

            char exe_path[MAX_PATH]{};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, exe_path, &size)) {
                const char* filename = strrchr(exe_path, '\\');
                if (filename) {
                    filename++;
                    for (int j = 0; tools[j] != nullptr; ++j) {
                        if (_stricmp(filename, tools[j]) == 0) {
                            found = true;
                            break;
                        }
                    }
                }
            }
            CloseHandle(hProcess);
        }

        return found;
    }

    // NtSetInformationThread with ThreadHideFromDebugger
    static void hide_from_debugger() {
        typedef LONG (NTAPI* NtSetInformationThread_t)(HANDLE, ULONG, PVOID, ULONG);

        auto NtSetInformationThread = reinterpret_cast<NtSetInformationThread_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread"));

        if (NtSetInformationThread) {
            NtSetInformationThread(GetCurrentThread(), 0x11, nullptr, 0);
        }
    }

    static bool should_terminate() {
        return is_debugger_present() ||
               check_peb_debugger() ||
               check_nt_global_flag() ||
               check_hardware_breakpoints() ||
               check_analysis_tools() ||
               check_timing();
    }
};

class AntiVM {
public:
    static bool check_vm_registry() {
        const char* vm_keys[] = {
            "SOFTWARE\\VMware, Inc.\\VMware Tools",
            "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
            "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
            "SYSTEM\\CurrentControlSet\\Services\\vmci",
            "SYSTEM\\CurrentControlSet\\Services\\vmhgfs",
            "SYSTEM\\CurrentControlSet\\Services\\VBoxMouse",
            "SYSTEM\\CurrentControlSet\\Services\\VBoxSF",
            nullptr
        };

        for (int i = 0; vm_keys[i] != nullptr; ++i) {
            HKEY hkey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, vm_keys[i], 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
                RegCloseKey(hkey);
                return true;
            }
        }
        return false;
    }

    static bool check_vm_files() {
        const char* vm_files[] = {
            "C:\\Windows\\System32\\vmGuestLib.dll",
            "C:\\Windows\\System32\\vm3dum.dll",
            "C:\\Windows\\System32\\VBoxHook.dll",
            "C:\\Windows\\System32\\SbieDll.dll",
            "C:\\Windows\\System32\\VBoxGuest.sys",
            "C:\\Windows\\System32\\VBoxTray.exe",
            "C:\\Windows\\System32\\vmtoolsd.exe",
            "C:\\Windows\\System32\\vmwaretray.exe",
            nullptr
        };

        for (int i = 0; vm_files[i] != nullptr; ++i) {
            DWORD attr = GetFileAttributesA(vm_files[i]);
            if (attr != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
        }
        return false;
    }

    // Check for VM processes using EnumProcesses
    static bool check_vm_processes() {
        const char* vm_dlls[] = {
            "vmGuestLib.dll", "vm3dum.dll", "VBoxHook.dll",
            "SbieDll.dll",
            nullptr
        };

        for (int i = 0; vm_dlls[i] != nullptr; ++i) {
            HMODULE hMod = GetModuleHandleA(vm_dlls[i]);
            if (hMod) return true;
        }

        // Check via process enumeration
        DWORD processes[1024]{};
        DWORD cb_needed = 0;
        if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) return false;

        DWORD num_processes = cb_needed / sizeof(DWORD);
        const char* vm_procs[] = {
            "vmtoolsd.exe", "vmwaretray.exe", "vmware.exe",
            "VBoxService.exe", "VBoxTray.exe",
            "qemu-ga.exe", "vdagent.exe",
            nullptr
        };

        bool found = false;
        for (DWORD i = 0; i < num_processes && !found; ++i) {
            if (processes[i] == 0) continue;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
            if (!hProcess) continue;

            char exe_path[MAX_PATH]{};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, exe_path, &size)) {
                const char* filename = strrchr(exe_path, '\\');
                if (filename) {
                    filename++;
                    for (int j = 0; vm_procs[j] != nullptr; ++j) {
                        if (_stricmp(filename, vm_procs[j]) == 0) {
                            found = true;
                            break;
                        }
                    }
                }
            }
            CloseHandle(hProcess);
        }

        return found;
    }

    static bool check_cpuid_hypervisor() {
        int cpuInfo[4]{};
        __cpuid(cpuInfo, 1);
        return (cpuInfo[2] & (1 << 31)) != 0;
    }

    static bool check_vm_bios() {
        HKEY hkey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\BIOS",
            0, KEY_READ, &hkey) == ERROR_SUCCESS) {

            char value[256]{};
            DWORD size = sizeof(value);
            DWORD type = 0;

            const char* bios_values[] = {
                "SystemManufacturer", "SystemProductName",
                "BIOSVendor", "BIOSVersion",
                nullptr
            };

            for (int i = 0; bios_values[i] != nullptr; ++i) {
                size = sizeof(value);
                if (RegQueryValueExA(hkey, bios_values[i], nullptr, &type,
                    reinterpret_cast<LPBYTE>(value), &size) == ERROR_SUCCESS) {
                    const char* vm_strings[] = {
                        "vmware", "virtualbox", "vbox", "qemu",
                        "xen", "hyper-v", "microsoft corporation",
                        nullptr
                    };
                    for (int j = 0; vm_strings[j] != nullptr; ++j) {
                        if (_stricmp(value, vm_strings[j]) == 0) {
                            RegCloseKey(hkey);
                            return true;
                        }
                    }
                }
            }
            RegCloseKey(hkey);
        }
        return false;
    }

    static bool is_virtual_machine() {
        return check_cpuid_hypervisor() ||
               check_vm_registry() ||
               check_vm_files() ||
               check_vm_processes() ||
               check_vm_bios();
    }
};

} // namespace nuub::domain::anti
#endif
