#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IProcessService.hpp"

namespace nuub::application::commands {

class ProcessHandler {
    interfaces::IProcessService& process_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    ProcessHandler(
        interfaces::IProcessService& process,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_ps(const std::string& target);
    domain::Result<void> handle_kill(const std::string& target, const std::string& pid_str);
};

} // namespace nuub::application::commands
