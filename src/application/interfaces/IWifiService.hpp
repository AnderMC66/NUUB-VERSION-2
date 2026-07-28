#pragma once

#include <string>
#include <vector>

namespace nuub::application::interfaces {

struct WifiAccessPoint {
    std::string ssid;
    std::string bssid;
    int signal_percent = 0;
};

class IWifiService {
public:
    virtual ~IWifiService() = default;

    [[nodiscard]] virtual std::string get_saved_networks() = 0;
    [[nodiscard]] virtual std::vector<WifiAccessPoint> get_visible_networks() = 0;
};

} // namespace nuub::application::interfaces
