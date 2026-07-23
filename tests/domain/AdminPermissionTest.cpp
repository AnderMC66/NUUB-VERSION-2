#include <gtest/gtest.h>

#include "domain/entities/Admin.hpp"

using namespace nuub::domain::entities;

class AdminPermissionTest : public ::testing::Test {
protected:
    Admin create_admin_with_roles() {
        std::vector<AdminInfo> roles = {
            {100, "full_user", Permission::FULL},
            {200, "limited_user", Permission::LIMITED},
            {300, "readonly_user", Permission::READONLY},
        };
        return Admin(std::move(roles));
    }
};

// ── Permission levels ─────────────────────────────────────────

TEST_F(AdminPermissionTest, FullAdminCanExecuteAllCommands) {
    auto admin = create_admin_with_roles();

    // FULL admin can execute everything
    EXPECT_TRUE(admin.can_execute(100, "shell"));
    EXPECT_TRUE(admin.can_execute(100, "kill"));
    EXPECT_TRUE(admin.can_execute(100, "rm"));
    EXPECT_TRUE(admin.can_execute(100, "downloadexec"));
    EXPECT_TRUE(admin.can_execute(100, "uninstall"));
    EXPECT_TRUE(admin.can_execute(100, "inject"));
    EXPECT_TRUE(admin.can_execute(100, "hollow"));
    EXPECT_TRUE(admin.can_execute(100, "shellcode"));
    EXPECT_TRUE(admin.can_execute(100, "status"));
    EXPECT_TRUE(admin.can_execute(100, "help"));
}

TEST_F(AdminPermissionTest, LimitedAdminCanExecuteLimitedCommands) {
    auto admin = create_admin_with_roles();

    // LIMITED can read + medium
    EXPECT_TRUE(admin.can_execute(200, "status"));
    EXPECT_TRUE(admin.can_execute(200, "shell"));
    EXPECT_TRUE(admin.can_execute(200, "wifi"));
    EXPECT_TRUE(admin.can_execute(200, "send"));
    EXPECT_TRUE(admin.can_execute(200, "screenshot"));

    // LIMITED cannot execute FULL commands
    EXPECT_FALSE(admin.can_execute(200, "kill"));
    EXPECT_FALSE(admin.can_execute(200, "rm"));
    EXPECT_FALSE(admin.can_execute(200, "downloadexec"));
    EXPECT_FALSE(admin.can_execute(200, "uninstall"));
    EXPECT_FALSE(admin.can_execute(200, "inject"));
}

TEST_F(AdminPermissionTest, ReadonlyAdminCanOnlyRead) {
    auto admin = create_admin_with_roles();

    // READONLY can read
    EXPECT_TRUE(admin.can_execute(300, "status"));
    EXPECT_TRUE(admin.can_execute(300, "sysinfo"));
    EXPECT_TRUE(admin.can_execute(300, "ps"));
    EXPECT_TRUE(admin.can_execute(300, "ls"));
    EXPECT_TRUE(admin.can_execute(300, "help"));
    EXPECT_TRUE(admin.can_execute(300, "locate"));

    // READONLY cannot execute LIMITED or FULL commands
    EXPECT_FALSE(admin.can_execute(300, "shell"));
    EXPECT_FALSE(admin.can_execute(300, "wifi"));
    EXPECT_FALSE(admin.can_execute(300, "send"));
    EXPECT_FALSE(admin.can_execute(300, "kill"));
    EXPECT_FALSE(admin.can_execute(300, "rm"));
    EXPECT_FALSE(admin.can_execute(300, "uninstall"));
}

// ── Getters ───────────────────────────────────────────────────

TEST_F(AdminPermissionTest, GetPermissionReturnsCorrectLevel) {
    auto admin = create_admin_with_roles();

    EXPECT_EQ(admin.get_permission(100), Permission::FULL);
    EXPECT_EQ(admin.get_permission(200), Permission::LIMITED);
    EXPECT_EQ(admin.get_permission(300), Permission::READONLY);
}

TEST_F(AdminPermissionTest, UnknownAdminReturnsReadonly) {
    auto admin = create_admin_with_roles();
    EXPECT_EQ(admin.get_permission(999), Permission::READONLY);
}

TEST_F(AdminPermissionTest, GetPermissionNameReturnsString) {
    auto admin = create_admin_with_roles();

    EXPECT_EQ(admin.get_permission_name(100), "full");
    EXPECT_EQ(admin.get_permission_name(200), "limited");
    EXPECT_EQ(admin.get_permission_name(300), "readonly");
    // Unknown admins default to readonly (safe default)
    EXPECT_EQ(admin.get_permission_name(999), "readonly");
}

// ── Required permission for commands ──────────────────────────

TEST_F(AdminPermissionTest, DestructiveCommandsRequireFull) {
    EXPECT_EQ(Admin::required_permission("kill"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("rm"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("downloadexec"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("uninstall"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("inject"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("hollow"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("shellcode"), Permission::FULL);
    EXPECT_EQ(Admin::required_permission("shutdown"), Permission::FULL);
}

TEST_F(AdminPermissionTest, MediumCommandsRequireLimited) {
    EXPECT_EQ(Admin::required_permission("shell"), Permission::LIMITED);
    EXPECT_EQ(Admin::required_permission("wifi"), Permission::LIMITED);
    EXPECT_EQ(Admin::required_permission("send"), Permission::LIMITED);
    EXPECT_EQ(Admin::required_permission("screenshot"), Permission::LIMITED);
    EXPECT_EQ(Admin::required_permission("getlog"), Permission::LIMITED);
}

TEST_F(AdminPermissionTest, ReadOnlyCommandsRequireReadonly) {
    EXPECT_EQ(Admin::required_permission("status"), Permission::READONLY);
    EXPECT_EQ(Admin::required_permission("sysinfo"), Permission::READONLY);
    EXPECT_EQ(Admin::required_permission("ps"), Permission::READONLY);
    EXPECT_EQ(Admin::required_permission("ls"), Permission::READONLY);
    EXPECT_EQ(Admin::required_permission("help"), Permission::READONLY);
    EXPECT_EQ(Admin::required_permission("locate"), Permission::READONLY);
}

TEST_F(AdminPermissionTest, UnknownCommandRequiresFull) {
    EXPECT_EQ(Admin::required_permission("nonexistent"), Permission::FULL);
}

// ── Authorization (backward compat) ───────────────────────────

TEST_F(AdminPermissionTest, IsAuthorizedWorks) {
    auto admin = create_admin_with_roles();
    EXPECT_TRUE(admin.is_authorized(100));
    EXPECT_TRUE(admin.is_authorized(200));
    EXPECT_TRUE(admin.is_authorized(300));
    EXPECT_FALSE(admin.is_authorized(999));
}

TEST_F(AdminPermissionTest, ChatIdsReturnsAllIds) {
    auto admin = create_admin_with_roles();
    auto ids = admin.chat_ids();
    EXPECT_EQ(ids.size(), 3u);
}

TEST_F(AdminPermissionTest, PrimaryChatIdIsFirst) {
    auto admin = create_admin_with_roles();
    EXPECT_EQ(admin.primary_chat_id(), 100);
}

// ── Legacy constructors ───────────────────────────────────────

TEST(AdminLegacyTest, SingleAdminGetsFull) {
    Admin admin(42);
    EXPECT_TRUE(admin.is_authorized(42));
    EXPECT_FALSE(admin.is_authorized(99));
    // Legacy single admin should have FULL access
    EXPECT_EQ(admin.get_permission(42), Permission::FULL);
}

TEST(AdminLegacyTest, MultipleAdminsAllGetFull) {
    Admin admin({100, 200});
    EXPECT_TRUE(admin.is_authorized(100));
    EXPECT_TRUE(admin.is_authorized(200));
    EXPECT_EQ(admin.get_permission(100), Permission::FULL);
    EXPECT_EQ(admin.get_permission(200), Permission::FULL);
}
