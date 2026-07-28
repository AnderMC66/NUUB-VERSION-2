#pragma once

#include <string>
#include <vector>
#include "application/interfaces/IWifiService.hpp"

namespace nuub::tests::mocks {

class MockWifiService final : public application::interfaces::IWifiService {
public:
    std::string networks = "SSID: HomeWiFi\n  Key Content: password123\n\nSSID: OfficeNet\n  Key Content: office456";
    std::vector<application::interfaces::WifiAccessPoint> visible_aps;
    std::string get_saved_networks() override { return networks; }
    std::vector<application::interfaces::WifiAccessPoint> get_visible_networks() override { return visible_aps; }
};

} // namespace nuub::tests::mocks
