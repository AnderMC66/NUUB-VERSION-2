#pragma once

#include <string>

namespace nuub::application::interfaces {

class IShellService {
public:
    virtual ~IShellService() = default;

    [[nodiscard]] virtual std::string execute(const std::string& command) = 0;
};

} // namespace nuub::application::interfaces
