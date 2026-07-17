#pragma once

#include <string>

namespace nuub::application::interfaces {

class ISysInfoService {
public:
    virtual ~ISysInfoService() = default;

    [[nodiscard]] virtual std::string get_system_info() = 0;
};

} // namespace nuub::application::interfaces
