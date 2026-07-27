#include <gtest/gtest.h>

#include "application/commands/ClipboardHandler.hpp"
#include "../mocks/MockClipboardService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class ClipboardHandlerTest : public ::testing::Test {
protected:
    MockClipboardService clipboard;
    MockReporter reporter;

    ClipboardHandler create_handler() {
        return ClipboardHandler(clipboard, reporter, "PC-Test");
    }
};

TEST_F(ClipboardHandlerTest, GetClipboard) {
    auto handler = create_handler();
    auto result = handler.handle_clipboard("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("mock clipboard content") != std::string::npos);
}

TEST_F(ClipboardHandlerTest, SetClipboard) {
    auto handler = create_handler();
    auto result = handler.handle_setclip("PC-Test", "new text");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(clipboard.clip_content, "new text");
}

TEST_F(ClipboardHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_clipboard("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(ClipboardHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_clipboard("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
