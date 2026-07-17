#include <gtest/gtest.h>

#include "application/commands/LocationHandler.hpp"
#include "../mocks/MockGeolocation.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class LocationHandlerTest : public ::testing::Test {
protected:
    MockGeolocation geolocation;
    MockReporter reporter;

    LocationHandler create_handler() {
        return LocationHandler(geolocation, reporter, "PC-Test");
    }
};

TEST_F(LocationHandlerTest, LocateReturnsLocation) {
    auto handler = create_handler();
    auto result = handler.handle_locate("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_GE(reporter.messages.size(), 2u);
    EXPECT_TRUE(reporter.messages[1].first.find("192.168.1.100") != std::string::npos);
    EXPECT_TRUE(reporter.messages[1].first.find("Madrid") != std::string::npos);
}

TEST_F(LocationHandlerTest, LocateIncludesMapsLink) {
    auto handler = create_handler();
    handler.handle_locate("PC-Test");
    EXPECT_GE(reporter.messages.size(), 2u);
    EXPECT_TRUE(reporter.messages[1].first.find("Google Maps") != std::string::npos);
}

TEST_F(LocationHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_locate("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(LocationHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_locate("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
