#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IWifiService.hpp"

namespace nuub::application::commands {

class WifiHandler {
    interfaces::IWifiService& wifi_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    WifiHandler(
        interfaces::IWifiService& wifi,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_wifi(const std::string& target);
};

} // namespace nuub::application::commands
