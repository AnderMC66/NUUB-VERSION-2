#pragma once

#include <string>

#include "application/interfaces/IShellService.hpp"

namespace nuub::tests::mocks {

class MockShellService final : public application::interfaces::IShellService {
public:
    std::string last_command;
    std::string response = "mock output";

    std::string execute(const std::string& command) override {
        last_command = command;
        return response;
    }
};

} // namespace nuub::tests::mocks
