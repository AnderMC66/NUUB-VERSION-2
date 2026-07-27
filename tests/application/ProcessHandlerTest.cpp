#include <gtest/gtest.h>

#include "application/commands/ProcessHandler.hpp"
#include "../mocks/MockProcessService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class ProcessHandlerTest : public ::testing::Test {
protected:
    MockProcessService process;
    MockReporter reporter;

    ProcessHandler create_handler() {
        return ProcessHandler(process, reporter, "PC-Test");
    }
};

TEST_F(ProcessHandlerTest, ListProcesses) {
    auto handler = create_handler();
    auto result = handler.handle_ps("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("test.exe") != std::string::npos);
}

TEST_F(ProcessHandlerTest, KillProcess) {
    auto handler = create_handler();
    auto result = handler.handle_kill("PC-Test", "1234");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(process.last_killed_pid, 1234);
}

TEST_F(ProcessHandlerTest, KillInvalidPidShowsError) {
    auto handler = create_handler();
    auto result = handler.handle_kill("PC-Test", "abc");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("PID") != std::string::npos);
}

TEST_F(ProcessHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_ps("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(ProcessHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_ps("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
