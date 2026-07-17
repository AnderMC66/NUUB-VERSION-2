#pragma once

#include <string>

#include "domain/common/Types.hpp"

namespace nuub::domain::entities {

struct ActivityEvent {
    std::string event_type;
    TimePoint timestamp;
    std::string pc_identifier;

    ActivityEvent(std::string type, std::string pc_id = "", TimePoint ts = now())
        : event_type(std::move(type)), timestamp(ts), pc_identifier(std::move(pc_id)) {}
};

} // namespace nuub::domain::entities
