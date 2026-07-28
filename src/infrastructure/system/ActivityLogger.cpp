#include "infrastructure/system/ActivityLogger.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define CTIME_S(buf, time) localtime_s(buf, time)
#else
#define CTIME_S(buf, time) localtime_r(time, buf)
#endif

namespace nuub::infrastructure::system {

ActivityLogger::ActivityLogger(std::string log_path, std::string pc_id)
    : log_path_(std::move(log_path)), pc_id_(std::move(pc_id)) {}

void ActivityLogger::register_event(const std::string& event_type) {
    std::lock_guard lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
    CTIME_S(&tm_buf, &time);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    std::ifstream check(log_path_);
    bool exists = check.good();
    check.close();

    std::ofstream file(log_path_, std::ios::app);
    if (!file.is_open()) return;

    if (!exists) {
        file << "Timestamp,PC_Identifier,Event\n";
    }
    file << timestamp << "," << pc_id_ << "," << event_type << "\n";
}

void ActivityLogger::register_event(const domain::entities::ActivityEvent& event) {
    std::lock_guard lock(mutex_);

    auto time = std::chrono::system_clock::to_time_t(event.timestamp);
    std::tm tm_buf{};
    CTIME_S(&tm_buf, &time);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();

    std::ifstream check(log_path_);
    bool exists = check.good();
    check.close();

    std::ofstream file(log_path_, std::ios::app);
    if (!file.is_open()) return;

    if (!exists) {
        file << "Timestamp,PC_Identifier,Event\n";
    }
    file << timestamp << "," << event.pc_identifier << "," << event.event_type << "\n";
}

} // namespace nuub::infrastructure::system
