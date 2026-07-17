#pragma once

#include <string>
#include <unordered_map>

namespace nuub::application::interfaces {

class IGeolocationService {
public:
    virtual ~IGeolocationService() = default;

    [[nodiscard]] virtual std::unordered_map<std::string, std::string> get_location() = 0;
};

} // namespace nuub::application::interfaces
