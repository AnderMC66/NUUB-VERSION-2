#include <gtest/gtest.h>

#include "domain/entities/Admin.hpp"

using namespace nuub::domain::entities;

TEST(AdminTest, AuthorizedChatId) {
    Admin admin(12345);
    EXPECT_TRUE(admin.is_authorized(12345));
}

TEST(AdminTest, UnauthorizedChatId) {
    Admin admin(12345);
    EXPECT_FALSE(admin.is_authorized(99999));
}

TEST(AdminTest, ChatIdAccessor) {
    Admin admin(42);
    EXPECT_EQ(admin.primary_chat_id(), 42);
}

TEST(AdminTest, MultipleAdmins) {
    Admin admin({100, 200, 300});
    EXPECT_TRUE(admin.is_authorized(100));
    EXPECT_TRUE(admin.is_authorized(200));
    EXPECT_TRUE(admin.is_authorized(300));
    EXPECT_FALSE(admin.is_authorized(400));
}

TEST(AdminTest, MultipleAdminsPrimaryId) {
    Admin admin({100, 200, 300});
    EXPECT_EQ(admin.primary_chat_id(), 100);
}

TEST(AdminTest, MultipleAdminsAccessor) {
    Admin admin({100, 200, 300});
    EXPECT_EQ(admin.chat_ids().size(), 3u);
}
