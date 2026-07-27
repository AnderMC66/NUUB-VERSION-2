#include <gtest/gtest.h>

#include "application/commands/FileManagerHandler.hpp"
#include "../mocks/MockFileManagerService.hpp"
#include "../mocks/MockReporter.hpp"

using namespace nuub::application::commands;
using namespace nuub::tests::mocks;

class FileManagerHandlerTest : public ::testing::Test {
protected:
    MockFileManagerService filemgr;
    MockReporter reporter;

    FileManagerHandler create_handler() {
        return FileManagerHandler(filemgr, reporter, "PC-Test");
    }
};

TEST_F(FileManagerHandlerTest, ListDirectory) {
    auto handler = create_handler();
    auto result = handler.handle_ls("PC-Test", "C:\\Users");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    EXPECT_TRUE(reporter.messages[0].first.find("file1.txt") != std::string::npos);
}

TEST_F(FileManagerHandlerTest, MakeDirectory) {
    auto handler = create_handler();
    auto result = handler.handle_mkdir("PC-Test", "new_dir");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("creado") != std::string::npos);
}

TEST_F(FileManagerHandlerTest, DeleteFile) {
    auto handler = create_handler();
    auto result = handler.handle_rm("PC-Test", "file.txt");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("Eliminado") != std::string::npos);
}

TEST_F(FileManagerHandlerTest, ReadFile) {
    auto handler = create_handler();
    auto result = handler.handle_cat("PC-Test", "file.txt");
    EXPECT_TRUE(result.is_success());
    EXPECT_TRUE(reporter.messages[0].first.find("file content here") != std::string::npos);
}

TEST_F(FileManagerHandlerTest, SendFile) {
    auto handler = create_handler();
    auto result = handler.handle_send("PC-Test", "report.xlsx");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.files.empty());
}

TEST_F(FileManagerHandlerTest, EmptyPathListsCurrentDir) {
    auto handler = create_handler();
    auto result = handler.handle_ls("PC-Test", "");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
    // Empty path defaults to "." and lists current directory
    EXPECT_TRUE(reporter.messages[0].first.find("file1.txt") != std::string::npos);
}

TEST_F(FileManagerHandlerTest, WrongTargetIgnored) {
    auto handler = create_handler();
    handler.handle_ls("Other-PC", "C:\\");
    EXPECT_TRUE(reporter.messages.empty());
}

TEST_F(FileManagerHandlerTest, AllTargetsMatch) {
    auto handler = create_handler();
    auto result = handler.handle_ls("all", ".");
    EXPECT_TRUE(result.is_success());
    EXPECT_FALSE(reporter.messages.empty());
}
