#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IShellService.hpp"

namespace nuub::application::commands {

class ShellHandler {
    interfaces::IShellService& shell_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    ShellHandler(
        interfaces::IShellService& shell,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_shell(const std::string& target, const std::string& command);
};

} // namespace nuub::application::commands
