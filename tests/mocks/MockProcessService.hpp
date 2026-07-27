#pragma once

#include <string>
#include "application/interfaces/IProcessService.hpp"

namespace nuub::tests::mocks {

class MockProcessService final : public application::interfaces::IProcessService {
public:
    std::string process_list = "PID 1234, Name: test.exe\nPID 5678, Name: explorer.exe";
    bool kill_result = true;
    std::string list_processes() override { return process_list; }
    bool kill_process(int pid) override {
        last_killed_pid = pid;
        return kill_result;
    }
    int last_killed_pid = 0;
};

} // namespace nuub::tests::mocks
