#pragma once

#include <string>
#include "application/interfaces/IDownloadExecService.hpp"

namespace nuub::tests::mocks {

class MockDownloadExecService final : public application::interfaces::IDownloadExecService {
public:
    std::string response = "File downloaded and executed: test.exe";
    std::string last_url;
    std::string download_and_execute(const std::string& url) override {
        last_url = url;
        return response;
    }
};

} // namespace nuub::tests::mocks
