#pragma once

#include <string>
#include "application/interfaces/IWifiService.hpp"

namespace nuub::tests::mocks {

class MockWifiService final : public application::interfaces::IWifiService {
public:
    std::string networks = "SSID: HomeWiFi\n  Key Content: password123\n\nSSID: OfficeNet\n  Key Content: office456";
    std::string get_saved_networks() override { return networks; }
};

} // namespace nuub::tests::mocks
