#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/ISysInfoService.hpp"

namespace nuub::application::commands {

class SysInfoHandler {
    interfaces::ISysInfoService& sysinfo_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    SysInfoHandler(
        interfaces::ISysInfoService& sysinfo,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_sysinfo(const std::string& target);
};

} // namespace nuub::application::commands
