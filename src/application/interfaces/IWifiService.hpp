#pragma once

#include <string>

namespace nuub::application::interfaces {

class IWifiService {
public:
    virtual ~IWifiService() = default;

    [[nodiscard]] virtual std::string get_saved_networks() = 0;
};

} // namespace nuub::application::interfaces
