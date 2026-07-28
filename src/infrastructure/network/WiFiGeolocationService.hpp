#pragma once

#include <string>
#include <unordered_map>

#include "application/interfaces/IGeolocationService.hpp"
#include "application/interfaces/IWifiService.hpp"

namespace nuub::infrastructure::network {

class WiFiGeolocationService final : public application::interfaces::IGeolocationService {
    application::interfaces::IWifiService& wifi_;
    std::string google_api_key_;

    [[nodiscard]] std::unordered_map<std::string, std::string> apple_wps_location(
        const std::vector<application::interfaces::WifiAccessPoint>& aps);
    [[nodiscard]] static std::unordered_map<std::string, std::string> windows_native_location();
    [[nodiscard]] std::unordered_map<std::string, std::string> google_wifi_location(
        const std::vector<application::interfaces::WifiAccessPoint>& aps);
    [[nodiscard]] std::unordered_map<std::string, std::string> fallback_ip_location();

public:
    explicit WiFiGeolocationService(
        application::interfaces::IWifiService& wifi,
        std::string google_api_key);

    [[nodiscard]] std::unordered_map<std::string, std::string> get_location() override;
};

} // namespace nuub::infrastructure::network