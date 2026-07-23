#include <gtest/gtest.h>

#include "domain/common/AuditLogger.hpp"

#include <fstream>
#include <string>
#include <cstdio>

using namespace nuub::domain;

class AuditLoggerTest : public ::testing::Test {
protected:
    std::string test_log = "test_audit.log";

    void SetUp() override {
        std::remove(test_log.c_str());
    }

    void TearDown() override {
        std::remove(test_log.c_str());
    }

    std::string read_log() {
        std::ifstream ifs(test_log);
        return std::string(std::istreambuf_iterator<char>(ifs),
                          std::istreambuf_iterator<char>());
    }
};

TEST_F(AuditLoggerTest, LogCommandCreatesEntry) {
    AuditLogger logger(test_log);
    logger.log_command("admin1", 12345, "full", "shell", "PC-001", "dir", true);

    std::string content = read_log();
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("admin1"), std::string::npos);
    EXPECT_NE(content.find("12345"), std::string::npos);
    EXPECT_NE(content.find("shell"), std::string::npos);
    EXPECT_NE(content.find("PC-001"), std::string::npos);
    EXPECT_NE(content.find("allowed=yes"), std::string::npos);
}

TEST_F(AuditLoggerTest, LogDeniedCommand) {
    AuditLogger logger(test_log);
    logger.log_denied(99999, "readonly", "kill");

    std::string content = read_log();
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("99999"), std::string::npos);
    EXPECT_NE(content.find("kill"), std::string::npos);
    EXPECT_NE(content.find("allowed=NO"), std::string::npos);
}

TEST_F(AuditLoggerTest, LogEventCreatesEntry) {
    AuditLogger logger(test_log);
    logger.log_event("AGENT_START pc=PC-001");

    std::string content = read_log();
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("AGENT_START"), std::string::npos);
    EXPECT_NE(content.find("PC-001"), std::string::npos);
}

TEST_F(AuditLoggerTest, MultipleEntriesAppend) {
    AuditLogger logger(test_log);
    logger.log_event("START");
    logger.log_command("admin", 1, "full", "status", "pc", "", true);
    logger.log_denied(2, "readonly", "kill");

    std::string content = read_log();
    // Count newlines to verify multiple entries
    int lines = 0;
    for (char c : content) {
        if (c == '\n') lines++;
    }
    EXPECT_GE(lines, 3);
}

TEST_F(AuditLoggerTest, LogWithExtraField) {
    AuditLogger logger(test_log);
    logger.log_command("admin", 1, "full", "shell", "pc", "dir /s", true);

    std::string content = read_log();
    EXPECT_NE(content.find("extra=dir /s"), std::string::npos);
}

TEST_F(AuditLoggerTest, TimestampFormat) {
    AuditLogger logger(test_log);
    logger.log_event("test");

    std::string content = read_log();
    // Timestamp format: YYYY-MM-DD HH:MM:SS.mmm
    EXPECT_NE(content.find("-"), std::string::npos);
    EXPECT_NE(content.find(":"), std::string::npos);
    EXPECT_NE(content.find("."), std::string::npos);
}
