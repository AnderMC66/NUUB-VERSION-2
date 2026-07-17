#include <gtest/gtest.h>

#include "domain/services/KeystrokeService.hpp"

using namespace nuub::domain::services;

TEST(KeystrokeServiceTest, InitiallyEmpty) {
    KeystrokeService ks;
    EXPECT_TRUE(ks.get_log().empty());
    EXPECT_FALSE(ks.is_paused());
}

TEST(KeystrokeServiceTest, PauseAndResume) {
    KeystrokeService ks;
    ks.pause();
    EXPECT_TRUE(ks.is_paused());

    // Key press while paused should be ignored
    ks.process_press("a");
    EXPECT_TRUE(ks.get_log().empty());

    ks.resume();
    EXPECT_FALSE(ks.is_paused());
}

TEST(KeystrokeServiceTest, ClearLogReturnsAndClears) {
    KeystrokeService ks;
    ks.process_press("a");
    ks.process_press("b");

    auto log = ks.clear_log();
    EXPECT_FALSE(log.empty());
    EXPECT_TRUE(ks.get_log().empty());
}

TEST(KeystrokeServiceTest, SpaceKey) {
    KeystrokeService ks;
    ks.process_press(" ");
    EXPECT_EQ(ks.get_log(), " ");
}

TEST(KeystrokeServiceTest, EmptyKeyIgnored) {
    KeystrokeService ks;
    ks.process_press("");
    EXPECT_TRUE(ks.get_log().empty());
}

TEST(KeystrokeServiceTest, MultipleKeysConcatenated) {
    KeystrokeService ks;
    ks.process_press("h");
    ks.process_press("e");
    ks.process_press("l");
    ks.process_press("l");
    ks.process_press("o");
    EXPECT_EQ(ks.get_log(), "hello");
}
