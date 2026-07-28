#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_set>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <cstdint>

#pragma comment(lib, "Psapi.lib")

namespace nuub::domain::perf {

// ═══════════════════════════════════════════════════════════════════════════════
// Process Cache — Enumerates processes ONCE, caches the result
// ═══════════════════════════════════════════════════════════════════════════════
class ProcessCache {
    std::mutex mutex_;
    std::unordered_set<std::string> process_names_;
    std::chrono::steady_clock::time_point last_refresh_;
    std::chrono::milliseconds refresh_interval_{5000}; // Refresh every 5 seconds

    ProcessCache() : last_refresh_(std::chrono::steady_clock::now()) {}

public:
    static ProcessCache& instance() {
        static ProcessCache inst;
        return inst;
    }

    // Get all running process names (cached, refreshes every N ms)
    std::unordered_set<std::string> get_process_names() {
        std::lock_guard lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        if (now - last_refresh_ >= refresh_interval_) {
            refresh();
            last_refresh_ = now;
        }
        return process_names_;
    }

    // Force refresh
    void refresh() {
        process_names_.clear();

        DWORD processes[2048]{};
        DWORD cb_needed = 0;
        if (!EnumProcesses(processes, sizeof(processes), &cb_needed)) return;

        DWORD num_processes = cb_needed / sizeof(DWORD);
        for (DWORD i = 0; i < num_processes; ++i) {
            if (processes[i] == 0) continue;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
            if (!hProcess) continue;

            char exe_path[MAX_PATH]{};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameA(hProcess, 0, exe_path, &size)) {
                const char* filename = strrchr(exe_path, '\\');
                if (filename) {
                    // Convert to lowercase for case-insensitive comparison
                    std::string name(filename + 1);
                    for (auto& c : name) c = static_cast<char>(::tolower(c));
                    process_names_.insert(std::move(name));
                }
            }
            CloseHandle(hProcess);
        }
    }

    // Check if a specific process is running (cached)
    bool is_running(const std::string& name) {
        std::string lower = name;
        for (auto& c : lower) c = static_cast<char>(::tolower(c));
        auto names = get_process_names();
        return names.count(lower) > 0;
    }

    // Check if ANY of the given processes are running
    bool any_running(const std::vector<std::string>& names) {
        auto cache = get_process_names();
        for (const auto& name : names) {
            std::string lower = name;
            for (auto& c : lower) c = static_cast<char>(::tolower(c));
            if (cache.count(lower)) return true;
        }
        return false;
    }

    void set_refresh_interval(std::chrono::milliseconds interval) {
        std::lock_guard lock(mutex_);
        refresh_interval_ = interval;
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Evasion Check Cache — Prevents redundant checks
// ═══════════════════════════════════════════════════════════════════════════════
class CheckCache {
    struct CachedResult {
        bool result = false;
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::milliseconds ttl{30000}; // 30 second TTL
    };

    std::mutex mutex_;
    std::unordered_map<std::string, CachedResult> cache_;

public:
    static CheckCache& instance() {
        static CheckCache inst;
        return inst;
    }

    // Get cached result or compute if expired
    bool get_or_compute(const std::string& key,
                        std::function<bool()> compute,
                        std::chrono::milliseconds ttl = std::chrono::milliseconds(30000)) {
        std::lock_guard lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        auto it = cache_.find(key);
        if (it != cache_.end() && (now - it->second.timestamp) < it->second.ttl) {
            return it->second.result;
        }

        bool result = compute();
        cache_[key] = {result, now, ttl};
        return result;
    }

    // Invalidate a specific check
    void invalidate(const std::string& key) {
        std::lock_guard lock(mutex_);
        cache_.erase(key);
    }

    // Invalidate all checks
    void invalidate_all() {
        std::lock_guard lock(mutex_);
        cache_.clear();
    }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Adaptive Polling — Dynamically adjusts check intervals
// ═══════════════════════════════════════════════════════════════════════════════
class AdaptivePoller {
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::function<void()> check_fn_;
    std::chrono::milliseconds base_interval_;
    std::chrono::milliseconds current_interval_;
    std::chrono::milliseconds min_interval_{1000};   // Never faster than 1s
    std::chrono::milliseconds max_interval_{60000};  // Never slower than 60s
    int consecutive_clean_ = 0;

public:
    AdaptivePoller(std::function<void()> check_fn,
                   std::chrono::milliseconds base = std::chrono::milliseconds(5000))
        : check_fn_(std::move(check_fn))
        , base_interval_(base)
        , current_interval_(base) {}

    ~AdaptivePoller() { stop(); }

    void start() {
        if (running_) return;
        running_ = true;
        thread_ = std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(current_interval_);
                if (!running_) break;

                check_fn_();

                // Adaptive: if no issues found, slow down
                // (caller should call report_clean() or report_threat())
                consecutive_clean_++;
                if (consecutive_clean_ > 3) {
                    current_interval_ = (std::min)(
                        current_interval_ + std::chrono::milliseconds(1000),
                        max_interval_);
                }
            }
        });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    void report_threat() {
        consecutive_clean_ = 0;
        current_interval_ = base_interval_; // Reset to fast polling
    }

    void report_clean() {
        // Already handled in the loop
    }

    std::chrono::milliseconds current_interval() const { return current_interval_; }
    bool is_running() const { return running_; }
};

// ═══════════════════════════════════════════════════════════════════════════════
// Clipboard Optimizer — Reduces polling frequency intelligently
// ═══════════════════════════════════════════════════════════════════════════════
class ClipboardOptimizer {
    std::atomic<bool> running_{false};
    std::thread thread_;
    UINT last_sequence_ = 0;
    std::function<void(const std::string&, const std::string&)> on_change_;
    std::chrono::milliseconds poll_interval_{2000};  // Start at 2s (not 1s)
    std::chrono::milliseconds min_interval_{500};    // Never faster than 500ms
    std::chrono::milliseconds max_interval_{10000};  // Never slower than 10s
    int idle_counter_ = 0;

    static std::string get_foreground_window() {
        HWND hwnd = GetForegroundWindow();
        if (!hwnd) return "";
        wchar_t title[256]{};
        int len = GetWindowTextW(hwnd, title, 256);
        if (len == 0) return "";
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            result += static_cast<char>(title[i]);
        }
        return result;
    }

    static std::string read_clipboard_text() {
        if (!IsClipboardFormatAvailable(CF_TEXT) &&
            !IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            return "";
        }
        if (!OpenClipboard(nullptr)) return "";

        std::string result;
        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                auto* wstr = static_cast<const wchar_t*>(GlobalLock(hData));
                if (wstr) {
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

public:
    ClipboardOptimizer() = default;
    ~ClipboardOptimizer() { stop(); }

    void set_on_change(std::function<void(const std::string&, const std::string&)> cb) {
        on_change_ = std::move(cb);
    }

    void start() {
        if (running_) return;
        running_ = true;
        last_sequence_ = GetClipboardSequenceNumber();
        thread_ = std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(poll_interval_);
                if (!running_) break;

                UINT current = GetClipboardSequenceNumber();
                if (current != last_sequence_ && last_sequence_ != 0) {
                    // Clipboard changed — use fast interval
                    idle_counter_ = 0;
                    poll_interval_ = min_interval_;

                    std::string content = read_clipboard_text();
                    if (!content.empty() && content.size() <= 4000) {
                        std::string window = get_foreground_window();
                        if (on_change_) on_change_(window, content);
                    }
                } else {
                    // No change — gradually slow down
                    idle_counter_++;
                    if (idle_counter_ > 5) {
                        poll_interval_ = (std::min)(
                            poll_interval_ + std::chrono::milliseconds(500),
                            max_interval_);
                    }
                }
                last_sequence_ = current;
            }
        });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }
};

} // namespace nuub::domain::perf
#endif
