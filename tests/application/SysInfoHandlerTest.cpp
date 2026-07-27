#include <gtest/gtest.h>

#include "application/commands/SysInfoHandler.hpp"
#include "../mocks/MockSysInfoService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class SysInfoHandlerTest : public ::testing::Test {
protected:
    MockSysInfoService sysinfo;
    MockReporter reporter;

    SysInfoHandler create_handler() {
        return SysInfoHandler(sysinfo, reporter, "PC-Test");
    }
};

TEST_F(SysInfoHandlerTest, ReturnsSystemInfo) {
    auto handler = create_handler();
    auto result = handler.handle_sysinfo("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("Mock System Info") != std::string::npos);
}

TEST_F(SysInfoHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_sysinfo("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(SysInfoHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_sysinfo("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
