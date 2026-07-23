#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace nuub::domain {

// Logs every command execution for accountability
class AuditLogger {
    std::string log_path_;
    std::mutex mutex_;

    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

public:
    explicit AuditLogger(const std::string& log_path)
        : log_path_(log_path) {}

    // Log a command execution
    void log_command(const std::string& admin_name,
                     std::int64_t chat_id,
                     const std::string& permission,
                     const std::string& command,
                     const std::string& target,
                     const std::string& extra = "",
                     bool allowed = true) {
        std::lock_guard lock(mutex_);

        std::ofstream ofs(log_path_, std::ios::app);
        if (!ofs.is_open()) return;

        ofs << timestamp()
            << " | admin=" << admin_name
            << " | chat_id=" << chat_id
            << " | perm=" << permission
            << " | cmd=" << command
            << " | target=" << target;
        if (!extra.empty()) {
            ofs << " | extra=" << extra;
        }
        ofs << " | allowed=" << (allowed ? "yes" : "NO")
            << "\n";
        ofs.flush();
    }

    // Log a denied command attempt
    void log_denied(std::int64_t chat_id,
                    const std::string& permission,
                    const std::string& command) {
        log_command("unknown", chat_id, permission, command, "", "", false);
    }

    // Log agent events (startup, shutdown, etc.)
    void log_event(const std::string& event) {
        std::lock_guard lock(mutex_);

        std::ofstream ofs(log_path_, std::ios::app);
        if (!ofs.is_open()) return;

        ofs << timestamp()
            << " | event=" << event
            << "\n";
        ofs.flush();
    }
};

} // namespace nuub::domain
