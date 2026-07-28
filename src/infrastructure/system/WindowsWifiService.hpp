#pragma once

#include <string>
#include <vector>

#include "application/interfaces/IWifiService.hpp"

namespace nuub::infrastructure::system {

class WindowsWifiService final : public application::interfaces::IWifiService {
public:
    [[nodiscard]] std::string get_saved_networks() override;
    [[nodiscard]] std::vector<application::interfaces::WifiAccessPoint> get_visible_networks() override;
};

} // namespace nuub::infrastructure::system
