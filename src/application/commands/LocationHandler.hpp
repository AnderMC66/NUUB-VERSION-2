#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IGeolocationService.hpp"

namespace nuub::application::commands {

class LocationHandler {
    interfaces::IGeolocationService& geolocation_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    LocationHandler(
        interfaces::IGeolocationService& geolocation,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_locate(const std::string& target);
};

} // namespace nuub::application::commands
