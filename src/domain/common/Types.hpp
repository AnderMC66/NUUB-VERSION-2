#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace nuub::domain {
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    inline TimePoint now() {
        return Clock::now();
    }

    inline std::int64_t timestamp_ms(TimePoint tp) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    }
}
