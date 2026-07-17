#pragma once

#include <string>
#include <unordered_map>

#include "application/interfaces/IGeolocationService.hpp"

namespace nuub::tests::mocks {

class MockGeolocation final : public application::interfaces::IGeolocationService {
public:
    [[nodiscard]] std::unordered_map<std::string, std::string> get_location() override {
        return {
            {"ip", "192.168.1.100"},
            {"city", "Madrid"},
            {"region", "Madrid"},
            {"country", "ES"},
            {"maps_link", "https://maps.example.com/40.4,-3.7"}
        };
    }
};

} // namespace nuub::tests::mocks
