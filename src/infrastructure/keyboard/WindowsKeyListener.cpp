#include "infrastructure/keyboard/WindowsKeyListener.hpp"

namespace nuub::infrastructure::keyboard {

static WindowsKeyListener* g_instance = nullptr;

WindowsKeyListener::WindowsKeyListener(domain::services::IKeystrokeService& service)
    : service_(service) {
    g_instance = this;
}

WindowsKeyListener::~WindowsKeyListener() {
    stop();
    g_instance = nullptr;
}

LRESULT CALLBACK WindowsKeyListener::keyboard_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_instance) {
        auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            std::string key = g_instance->resolver_.resolve(
                static_cast<int>(kbd->vkCode),
                static_cast<int>(kbd->scanCode));
            g_instance->service_.process_press(key);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            std::string key = g_instance->resolver_.resolve(
                static_cast<int>(kbd->vkCode),
                static_cast<int>(kbd->scanCode));
            g_instance->service_.process_release(key);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void WindowsKeyListener::start() {
    if (running_) return;
    running_ = true;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_proc, nullptr, 0);
}

void WindowsKeyListener::stop() {
    running_ = false;
    if (hook_) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
}

} // namespace nuub::infrastructure::keyboard
