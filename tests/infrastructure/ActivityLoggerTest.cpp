#include <gtest/gtest.h>
#include <fstream>

#include "infrastructure/system/ActivityLogger.hpp"
#include "domain/entities/ActivityEvent.hpp"

using namespace nuub::infrastructure::system;

class ActivityLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        log_path_ = "test_activity_log.csv";
        pc_id_ = "PC-Test";
    }

    void TearDown() override {
        std::remove(log_path_.c_str());
    }

    std::string log_path_;
    std::string pc_id_;
};

TEST_F(ActivityLoggerTest, CreateLogger) {
    ActivityLogger logger(log_path_, pc_id_);
    EXPECT_NO_THROW(logger.register_event("TEST_EVENT"));

    std::ifstream ifs(log_path_);
    EXPECT_TRUE(ifs.is_open());
}

TEST_F(ActivityLoggerTest, LogEventByString) {
    ActivityLogger logger(log_path_, pc_id_);
    logger.register_event("STARTUP");

    std::ifstream ifs(log_path_);
    std::string line;
    std::getline(ifs, line); // skip header
    std::getline(ifs, line); // data line
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line.find("STARTUP"), std::string::npos);
    EXPECT_NE(line.find(pc_id_), std::string::npos);
}

TEST_F(ActivityLoggerTest, LogEventByObject) {
    ActivityLogger logger(log_path_, pc_id_);
    nuub::domain::entities::ActivityEvent event("CUSTOM_EVENT", pc_id_);
    logger.register_event(event);

    std::ifstream ifs(log_path_);
    std::string line;
    std::getline(ifs, line); // skip header
    std::getline(ifs, line); // data line
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line.find("CUSTOM_EVENT"), std::string::npos);
}

TEST_F(ActivityLoggerTest, MultipleEvents) {
    ActivityLogger logger(log_path_, pc_id_);
    logger.register_event("EVENT_A");
    logger.register_event("EVENT_B");
    logger.register_event("EVENT_C");

    std::ifstream ifs(log_path_);
    std::string line;
    int count = 0;
    while (std::getline(ifs, line)) {
        count++;
    }
    EXPECT_EQ(count, 4); // header + 3 events
}

TEST_F(ActivityLoggerTest, LogContainsPcId) {
    ActivityLogger logger(log_path_, pc_id_);
    logger.register_event("CONNECT");

    std::ifstream ifs(log_path_);
    std::string line;
    std::getline(ifs, line); // skip header
    std::getline(ifs, line); // data line
    EXPECT_NE(line.find("PC-Test"), std::string::npos);
}