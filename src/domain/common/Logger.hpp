#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace nuub::domain {

class Logger {
    inline static std::shared_ptr<spdlog::logger> instance_;

public:
    static void init(const std::string& log_file = "nuub.log") {
        std::vector<spdlog::sink_ptr> sinks;

        // File sink with rotation (5MB, 3 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 5 * 1024 * 1024, 3);
        sinks.push_back(file_sink);

        // Console sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        sinks.push_back(console_sink);

        instance_ = std::make_shared<spdlog::logger>("nuub", sinks.begin(), sinks.end());
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
