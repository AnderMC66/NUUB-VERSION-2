#pragma once

#include <mutex>
#include <string>

#include "domain/entities/ActivityEvent.hpp"

namespace nuub::infrastructure::system {

class ActivityLogger {
    std::string log_path_;
    std::string pc_id_;
    std::mutex mutex_;

public:
    ActivityLogger(std::string log_path, std::string pc_id);

    void register_event(const std::string& event_type);
    void register_event(const domain::entities::ActivityEvent& event);
};

} // namespace nuub::infrastructure::system
