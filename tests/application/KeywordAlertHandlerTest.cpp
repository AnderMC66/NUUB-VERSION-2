#include <gtest/gtest.h>

#include "application/commands/KeywordAlertHandler.hpp"
#include "domain/services/KeystrokeService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::domain::services;
using namespace nuub::tests::mocks;

class KeywordAlertHandlerTest : public ::testing::Test {
protected:
    KeystrokeService keystrokes;
    MockReporter reporter;

    KeywordAlertHandler create_handler() {
        return KeywordAlertHandler(keystrokes, reporter, "PC-Test");
    }
};

TEST_F(KeywordAlertHandlerTest, AddAlert) {
    auto handler = create_handler();
    auto result = handler.handle_add_alert("PC-Test", "password");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("password") != std::string::npos);
    auto kw = keystrokes.get_keywords();
    EXPECT_EQ(kw.size(), 1u);
}

TEST_F(KeywordAlertHandlerTest, RemoveAlert) {
    auto handler = create_handler();
    handler.handle_add_alert("PC-Test", "password");
    auto result = handler.handle_remove_alert("PC-Test", "password");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(keystrokes.get_keywords().empty());
}

TEST_F(KeywordAlertHandlerTest, ListAlerts) {
    auto handler = create_handler();
    handler.handle_add_alert("PC-Test", "password");
    handler.handle_add_alert("PC-Test", "secret");
    auto result = handler.handle_list_alerts("PC-Test");
    EXPECT_TRUE(result.is_success());
    // messages[0] = "Alerta agregada: password"
    // messages[1] = "Alerta agregada: secret"
    // messages[2] = "Alertas activas:\n- password\n- secret\n"
    std::string all;
    for (const auto& m : reporter.messages) all += m.first;
    EXPECT_TRUE(all.find("password") != std::string::npos);
    EXPECT_TRUE(all.find("secret") != std::string::npos);
}

TEST_F(KeywordAlertHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_add_alert("Other-PC", "password");
    EXPECT_TRUE(reporter.messages.empty());
    EXPECT_TRUE(keystrokes.get_keywords().empty());
}

TEST_F(KeywordAlertHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_add_alert("all", "password");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
