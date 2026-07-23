#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <intrin.h>
#include <cstring>
#include "domain/common/PerformanceOptimizer.hpp"

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

    // Check for analysis tools (uses cached process list — no EnumProcesses per call)
    static bool check_analysis_tools() {
        static const std::vector<std::string> tools = {
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
            "dbgview.exe", "spyxx.exe"
        };
        return perf::ProcessCache::instance().any_running(tools);
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

    // Check for VM processes (uses cached process list)
    static bool check_vm_processes() {
        // First check loaded DLLs (instant, no enumeration)
        static const std::vector<std::string> vm_dlls = {
            "vmGuestLib.dll", "vm3dum.dll", "VBoxHook.dll", "SbieDll.dll"
        };
        for (const auto& dll : vm_dlls) {
            if (GetModuleHandleA(dll.c_str())) return true;
        }

        // Then check running processes (cached)
        static const std::vector<std::string> vm_procs = {
            "vmtoolsd.exe", "vmwaretray.exe", "vmware.exe",
            "VBoxService.exe", "VBoxTray.exe",
            "qemu-ga.exe", "vdagent.exe"
        };
        return perf::ProcessCache::instance().any_running(vm_procs);
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
