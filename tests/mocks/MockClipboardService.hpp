#pragma once

#include <string>
#include "application/interfaces/IClipboardService.hpp"

namespace nuub::tests::mocks {

class MockClipboardService final : public application::interfaces::IClipboardService {
public:
    std::string clip_content = "mock clipboard content";
    std::string get_clipboard() override { return clip_content; }
    void set_clipboard(const std::string& text) override { clip_content = text; }
};

} // namespace nuub::tests::mocks
