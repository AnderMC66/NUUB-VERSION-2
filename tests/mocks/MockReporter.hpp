#pragma once

#include <string>
#include <vector>

#include "application/interfaces/IReporter.hpp"

namespace nuub::tests::mocks {

class MockReporter final : public application::interfaces::IReporter {
public:
    std::vector<std::pair<std::string, std::string>> messages;
    std::vector<std::pair<std::string, std::string>> files;
    bool fail_send = false;

    void send_message(const std::string& text) override {
        messages.emplace_back(text, "");
    }

    bool send_file(const std::string& path, const std::string& caption = "") override {
        if (fail_send) return false;
        files.emplace_back(path, caption);
        return true;
    }
};

} // namespace nuub::tests::mocks
