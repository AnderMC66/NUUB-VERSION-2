#pragma once

#include <string>

#include "application/interfaces/IShellService.hpp"

namespace nuub::infrastructure::system {

class ShellService final : public application::interfaces::IShellService {
public:
    [[nodiscard]] std::string execute(const std::string& command) override;
};

} // namespace nuub::infrastructure::system
