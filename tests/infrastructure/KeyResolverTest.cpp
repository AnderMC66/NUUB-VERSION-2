#include <gtest/gtest.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "infrastructure/keyboard/KeyResolver.hpp"

using namespace nuub::infrastructure::keyboard;

class KeyResolverTest : public ::testing::Test {
protected:
    KeyResolver resolver;
};

TEST_F(KeyResolverTest, SpaceKey) {
    EXPECT_EQ(resolver.resolve(VK_SPACE, 0), " ");
}

TEST_F(KeyResolverTest, ReturnKey) {
    EXPECT_EQ(resolver.resolve(VK_RETURN, 0), "\n");
}

TEST_F(KeyResolverTest, TabKey) {
    EXPECT_EQ(resolver.resolve(VK_TAB, 0), "\t");
}

TEST_F(KeyResolverTest, BackspaceKey) {
    EXPECT_EQ(resolver.resolve(VK_BACK, 0), "\x08");
}

TEST_F(KeyResolverTest, EscapeKey) {
    EXPECT_EQ(resolver.resolve(VK_ESCAPE, 0), " [escape] ");
}

TEST_F(KeyResolverTest, DeleteKey) {
    EXPECT_EQ(resolver.resolve(VK_DELETE, 0), " [delete] ");
}

TEST_F(KeyResolverTest, ArrowKeys) {
    EXPECT_EQ(resolver.resolve(VK_UP, 0), " [up] ");
    EXPECT_EQ(resolver.resolve(VK_DOWN, 0), " [down] ");
    EXPECT_EQ(resolver.resolve(VK_LEFT, 0), " [left] ");
    EXPECT_EQ(resolver.resolve(VK_RIGHT, 0), " [right] ");
}

TEST_F(KeyResolverTest, NavigationKeys) {
    EXPECT_EQ(resolver.resolve(VK_HOME, 0), " [home] ");
    EXPECT_EQ(resolver.resolve(VK_END, 0), " [end] ");
    EXPECT_EQ(resolver.resolve(VK_PRIOR, 0), " [pageup] ");
    EXPECT_EQ(resolver.resolve(VK_NEXT, 0), " [pagedown] ");
}

TEST_F(KeyResolverTest, FunctionKeys) {
    EXPECT_EQ(resolver.resolve(VK_F1, 0), " [f1] ");
    EXPECT_EQ(resolver.resolve(VK_F5, 0), " [f5] ");
    EXPECT_EQ(resolver.resolve(VK_F12, 0), " [f12] ");
}

TEST_F(KeyResolverTest, AlphanumericKeys) {
    EXPECT_EQ(resolver.resolve('A', 0), "a");
    EXPECT_EQ(resolver.resolve('Z', 0), "z");
    EXPECT_EQ(resolver.resolve('0', 0), "0");
    EXPECT_EQ(resolver.resolve('9', 0), "9");
}

TEST_F(KeyResolverTest, ModifierKeysReturnEmpty) {
    EXPECT_TRUE(resolver.resolve(VK_SHIFT, 0).empty());
    EXPECT_TRUE(resolver.resolve(VK_CONTROL, 0).empty());
    EXPECT_TRUE(resolver.resolve(VK_MENU, 0).empty());
    EXPECT_TRUE(resolver.resolve(VK_LWIN, 0).empty());
    EXPECT_TRUE(resolver.resolve(VK_LSHIFT, 0).empty());
}

TEST_F(KeyResolverTest, CapsLockToggles) {
    resolver.handle_release(VK_CAPITAL);
    EXPECT_TRUE(resolver.resolve(VK_CAPITAL, 0).empty());
}

TEST_F(KeyResolverTest, HandleReleaseDoesNotCrash) {
    EXPECT_NO_THROW(resolver.handle_release(VK_SPACE));
    EXPECT_NO_THROW(resolver.handle_release('A'));
    EXPECT_NO_THROW(resolver.handle_release(VK_RETURN));
}

TEST_F(KeyResolverTest, UnknownKeyCode) {
    EXPECT_NO_THROW(resolver.resolve(0xFF, 0));
}