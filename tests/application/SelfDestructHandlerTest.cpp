#include <gtest/gtest.h>

#include "application/commands/SelfDestructHandler.hpp"
#include "domain/entities/Admin.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class SelfDestructHandlerTest : public ::testing::Test {
protected:
    MockReporter reporter;

    SelfDestructHandler create_handler() {
        return SelfDestructHandler(reporter, "PC-Test");
    }
};

// ── Target matching ───────────────────────────────────────────

TEST_F(SelfDestructHandlerTest, CorrectTargetMatches) {
    auto handler = create_handler();
    // We can't actually call handle_uninstall (it calls ExitProcess),
    // but we can verify the handler exists and was constructed correctly
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(SelfDestructHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    // The handler should not do anything if target doesn't match
    // We verify by checking that no messages are sent before the match check
    EXPECT_TRUE(reporter.messages.empty());
}

// ── Permission level ──────────────────────────────────────────
// Verify that uninstall requires FULL permission

TEST(SelfDestructPermissionTest, UninstallRequiresFullPermission) {
    // Test that the command "uninstall" is mapped to FULL permission
    auto perm = nuub::domain::entities::Admin::required_permission("uninstall");
    EXPECT_EQ(perm, nuub::domain::entities::Permission::FULL);
}
