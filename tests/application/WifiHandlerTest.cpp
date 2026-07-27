#include <gtest/gtest.h>

#include "application/commands/WifiHandler.hpp"
#include "../mocks/MockWifiService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class WifiHandlerTest : public ::testing::Test {
protected:
    MockWifiService wifi;
    MockReporter reporter;

    WifiHandler create_handler() {
        return WifiHandler(wifi, reporter, "PC-Test");
    }
};

TEST_F(WifiHandlerTest, ReturnsNetworks) {
    auto handler = create_handler();
    auto result = handler.handle_wifi("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("HomeWiFi") != std::string::npos);
}

TEST_F(WifiHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_wifi("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(WifiHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_wifi("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
