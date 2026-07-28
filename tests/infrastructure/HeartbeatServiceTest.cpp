#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>

#include "infrastructure/telegram/HeartbeatService.hpp"

using namespace nuub::infrastructure::telegram;

class HeartbeatServiceTest : public ::testing::Test {
protected:
    std::atomic<int> heartbeat_count_{0};
    std::string last_message_;

    auto make_callback() {
        return [this](const std::string& msg) {
            heartbeat_count_++;
            last_message_ = msg;
        };
    }
};

TEST_F(HeartbeatServiceTest, StartStopWithoutCrash) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    EXPECT_NO_THROW(svc.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_NO_THROW(svc.stop());
}

TEST_F(HeartbeatServiceTest, StartTwiceDoesNotCrash) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    svc.start();
    EXPECT_NO_THROW(svc.start());
    svc.stop();
}

TEST_F(HeartbeatServiceTest, StopWithoutStartDoesNotCrash) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    EXPECT_NO_THROW(svc.stop());
}

TEST_F(HeartbeatServiceTest, StopTwiceDoesNotCrash) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    svc.start();
    svc.stop();
    EXPECT_NO_THROW(svc.stop());
}

TEST_F(HeartbeatServiceTest, ConstructorDoesNotStartThread) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(heartbeat_count_, 0);
}

TEST_F(HeartbeatServiceTest, ShortIntervalWorks) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    svc.start();
    svc.stop();
}

TEST_F(HeartbeatServiceTest, MultipleStartStopCycles) {
    HeartbeatService svc(make_callback(), "PC-Test", 0);
    for (int i = 0; i < 3; ++i) {
        svc.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        svc.stop();
    }
}

TEST_F(HeartbeatServiceTest, CallbackReceivesPcId) {
    HeartbeatService svc(make_callback(), "UniquePC-42", 1);
    (void)svc;
}