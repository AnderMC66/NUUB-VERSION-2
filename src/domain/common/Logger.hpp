#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace nuub::domain {

class Logger {
    inline static std::shared_ptr<spdlog::logger> instance_;

public:
    // File-only logger — no console output in production
    static void init(const std::string& log_file = "agent.log") {
        std::vector<spdlog::sink_ptr> sinks;

        // File sink with rotation (5MB, 3 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 5 * 1024 * 1024, 3);
        sinks.push_back(file_sink);

        // No console sink — avoids detection and stdout leakage

        instance_ = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
        instance_->set_level(spdlog::level::info);
        instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(instance_);
    }

    static std::shared_ptr<spdlog::logger> get() {
        if (!instance_) init();
        return instance_;
    }
};

} // namespace nuub::domain
