#include <gtest/gtest.h>

#include "application/commands/ShellHandler.hpp"
#include "../mocks/MockShellService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class ShellHandlerTest : public ::testing::Test {
protected:
    MockShellService shell;
    MockReporter reporter;

    ShellHandler create_handler() {
        return ShellHandler(shell, reporter, "PC-Test");
    }
};

TEST_F(ShellHandlerTest, ExecutesCommand) {
    auto handler = create_handler();
    auto result = handler.handle_shell("PC-Test", "dir");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(shell.last_command, "dir");
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(ShellHandlerTest, ReturnsOutput) {
    shell.response = "hello world";
    auto handler = create_handler();
    handler.handle_shell("PC-Test", "echo hello");
    EXPECT_TRUE(reporter.messages[1].first.find("hello world") != std::string::npos);
}

TEST_F(ShellHandlerTest, EmptyCommandShowsUsage) {
    auto handler = create_handler();
    auto result = handler.handle_shell("PC-Test", "");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(ShellHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_shell("Other-PC", "dir");
    EXPECT_TRUE(reporter.messages.empty());
    EXPECT_TRUE(shell.last_command.empty());
}

TEST_F(ShellHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_shell("all", "whoami");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(shell.last_command, "whoami");
}
