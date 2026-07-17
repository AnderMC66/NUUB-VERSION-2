#pragma once

#include <string>

#include "application/interfaces/ISysInfoService.hpp"

namespace nuub::infrastructure::system {

class WindowsSysInfoService final : public application::interfaces::ISysInfoService {
public:
    [[nodiscard]] std::string get_system_info() override;
};

} // namespace nuub::infrastructure::system
