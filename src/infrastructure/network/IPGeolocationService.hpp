#pragma once

#include <string>
#include <unordered_map>

#include "application/interfaces/IGeolocationService.hpp"

namespace nuub::infrastructure::network {

class IPGeolocationService final : public application::interfaces::IGeolocationService {
    static constexpr const char* API_URL = "https://ipinfo.io/json";

public:
    [[nodiscard]] std::unordered_map<std::string, std::string> get_location() override;
};

} // namespace nuub::infrastructure::network
