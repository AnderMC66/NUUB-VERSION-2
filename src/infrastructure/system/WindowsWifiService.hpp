#pragma once

#include <string>

#include "application/interfaces/IWifiService.hpp"

namespace nuub::infrastructure::system {

class WindowsWifiService final : public application::interfaces::IWifiService {
public:
    [[nodiscard]] std::string get_saved_networks() override;
};

} // namespace nuub::infrastructure::system
