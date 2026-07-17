#include <gtest/gtest.h>

#include "application/commands/MediaHandler.hpp"
#include "../mocks/MockReporter.hpp"
#include "../mocks/MockMediaCapture.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class MediaHandlerTest : public ::testing::Test {
protected:
    MockMediaCapture capture;
    MockReporter reporter;

    MediaHandler create_handler() {
        return MediaHandler(capture, reporter, "PC-Test");
    }
};

TEST_F(MediaHandlerTest, TakePhotoSuccess) {
    auto handler = create_handler();
    auto result = handler.handle_take_photo("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.files.empty());
}

TEST_F(MediaHandlerTest, TakePhotoFailure) {
    capture.fail_photo = true;
    auto handler = create_handler();
    auto result = handler.handle_take_photo("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.files.empty());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(MediaHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_take_photo("Other-PC");
    EXPECT_TRUE(reporter.files.empty());
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(MediaHandlerTest, VideoSuccess) {
    auto handler = create_handler();
    auto result = handler.handle_take_video("pc-test", 5);
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.files.empty());
}

TEST_F(MediaHandlerTest, AudioSuccess) {
    auto handler = create_handler();
    auto result = handler.handle_record_audio("PC-TEST", 3);
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.files.empty());
}

TEST_F(MediaHandlerTest, PhotoNotDeletedOnSendFailure) {
    capture.fail_photo = false;
    reporter.fail_send = true;
    auto handler = create_handler();
    auto result = handler.handle_take_photo("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.files.empty());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(MediaHandlerTest, ScreenshotSuccess) {
    auto handler = create_handler();
    auto result = handler.handle_screenshot("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.files.empty());
}

TEST_F(MediaHandlerTest, ScreenshotFailure) {
    capture.fail_screenshot = true;
    auto handler = create_handler();
    auto result = handler.handle_screenshot("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.files.empty());
    EXPECT_FALSE(reporter.messages.empty());
}

TEST_F(MediaHandlerTest, ScreenshotNotDeletedOnSendFailure) {
    capture.fail_screenshot = false;
    reporter.fail_send = true;
    auto handler = create_handler();
    auto result = handler.handle_screenshot("PC-Test");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.files.empty());
    EXPECT_FALSE(reporter.messages.empty());
}
