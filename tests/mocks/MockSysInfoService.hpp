#pragma once

#include <string>
#include "application/interfaces/ISysInfoService.hpp"

namespace nuub::tests::mocks {

class MockSysInfoService final : public application::interfaces::ISysInfoService {
public:
    std::string response = "Mock System Info: Windows 10, 16GB RAM";
    std::string get_system_info() override { return response; }
};

} // namespace nuub::tests::mocks
