#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <intrin.h>
#include <tlhelp32.h>

namespace nuub::domain::anti {

class AntiDebug {
public:
    // Check if debugger is present
    static bool is_debugger_present() {
        return IsDebuggerPresent() != 0;
    }

    // Check PEB->BeingDebugged directly
    static bool check_peb_debugger() {
        #ifdef _WIN64
        auto* peb = reinterpret_cast<uintptr_t*>(__readgsqword(0x60));
        #else
        auto* peb = reinterpret_cast<uintptr_t*>(__readfsdword(0x30));
        #endif
        return *reinterpret_cast<uint8_t*>(peb + 2) != 0;
    }

    // Check NtGlobalFlag (debug flags)
    static bool check_nt_global_flag() {
        #ifdef _WIN64
        auto* peb = reinterpret_cast<uint8_t*>(__readgsqword(0x60));
        #else
        auto* peb = reinterpret_cast<uint8_t*>(__readfsdword(0x30));
        #endif
        // Debug heap adds flags: 0x70 (FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS)
        uint32_t nt_global_flag = *reinterpret_cast<uint32_t*>(peb + 0xBC); // 64-bit offset
        return (nt_global_flag & 0x70) != 0;
    }

    // Check timing (single-step debugging)
    static bool check_timing() {
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);

        // Do some work
        volatile int x = 0;
        for (int i = 0; i < 1000; ++i) x += i;

        QueryPerformanceCounter(&end);

        double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
        // Normal execution should be microseconds, debugging adds milliseconds
        return elapsed > 0.01; // 10ms threshold
    }

    // Check for hardware breakpoints (DR registers)
    static bool check_hardware_breakpoints() {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            return ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0;
        }
        return false;
    }

    // Check for common analysis tools
    static bool check_analysis_tools() {
        const char* tools[] = {
            "ollydbg.exe", "x64dbg.exe", "x32dbg.exe",
            "ida.exe", "ida64.exe", "idag.exe",
            "wireshark.exe", "fiddler.exe",
            "procmon.exe", "procmon64.exe",
            "processhacker.exe", "processhacker.bin",
            "cheatengine.exe", "cheatengine-i386.exe",
            "dnspy.exe", "de4dot.exe",
            nullptr
        };

        for (int i = 0; tools[i] != nullptr; ++i) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0);
            if (hProcess) {
                char exe_path[MAX_PATH]{};
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageNameA(hProcess, 0, exe_path, &size)) {
                    // Extract filename
                    char* filename = strrchr(exe_path, '\\');
                    if (filename) {
                        filename++;
                        for (int j = 0; tools[j] != nullptr; ++j) {
                            if (_stricmp(filename, tools[j]) == 0) {
                                CloseHandle(hProcess);
                                return true;
                            }
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
        return false;
    }

    // Combined check
    static bool should_terminate() {
        return is_debugger_present() ||
               check_peb_debugger() ||
               check_hardware_breakpoints() ||
               check_analysis_tools();
    }
};

class AntiVM {
public:
    // Check for VM-specific registry keys
    static bool check_vm_registry() {
        const char* vm_keys[] = {
            "SOFTWARE\\VMware, Inc.\\VMware Tools",
            "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
            "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
            "SYSTEM\\CurrentControlSet\\Services\\vmci",
            "SYSTEM\\CurrentControlSet\\Services\\vmhgfs",
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

    // Check for VM-specific files
    static bool check_vm_files() {
        const char* vm_files[] = {
            "C:\\Windows\\System32\\vmGuestLib.dll",
            "C:\\Windows\\System32\\vm3dum.dll",
            "C:\\Windows\\System32\\VBoxHook.dll",
            "C:\\Windows\\System32\\SbieDll.dll", // Sandboxie
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

    // Check for VM-specific processes via WMI or simple file check
    static bool check_vm_processes() {
        // Check for VM-specific DLLs as proxy for VM processes
        const char* vm_dlls[] = {
            "vmGuestLib.dll", "vm3dum.dll", "VBoxHook.dll",
            "SbieDll.dll", // Sandboxie
            nullptr
        };

        for (int i = 0; vm_dlls[i] != nullptr; ++i) {
            HMODULE hMod = GetModuleHandleA(vm_dlls[i]);
            if (hMod) return true;
        }
        return false;
    }

    // Check CPUID for VM hypervisor bit
    static bool check_cpuid_hypervisor() {
        int cpuInfo[4]{};
        __cpuid(cpuInfo, 1);
        // Bit 31 of ECX indicates hypervisor present
        return (cpuInfo[2] & (1 << 31)) != 0;
    }

    // Combined check
    static bool is_virtual_machine() {
        return check_cpuid_hypervisor() ||
               check_vm_registry() ||
               check_vm_files() ||
               check_vm_processes();
    }
};

} // namespace nuub::domain::anti
#endif
