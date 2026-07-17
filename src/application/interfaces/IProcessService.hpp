#pragma once

#include <string>

namespace nuub::application::interfaces {

class IProcessService {
public:
    virtual ~IProcessService() = default;

    [[nodiscard]] virtual std::string list_processes() = 0;
    [[nodiscard]] virtual bool kill_process(int pid) = 0;
};

} // namespace nuub::application::interfaces
