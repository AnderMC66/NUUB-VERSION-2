#pragma once

#include <string>

#include "application/interfaces/IProcessService.hpp"

namespace nuub::infrastructure::system {

class WindowsProcessService final : public application::interfaces::IProcessService {
public:
    [[nodiscard]] std::string list_processes() override;
    [[nodiscard]] bool kill_process(int pid) override;
};

} // namespace nuub::infrastructure::system
