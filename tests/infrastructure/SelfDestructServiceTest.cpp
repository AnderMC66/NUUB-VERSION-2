#include <gtest/gtest.h>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "infrastructure/system/SelfDestructService.hpp"

using namespace nuub::infrastructure::system;

class SelfDestructServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test files
        {
            std::ofstream ofs("test_delete_me.txt");
            ofs << "delete me";
        }
        {
            std::ofstream ofs("test_delete_me2.txt");
            ofs << "delete me too";
        }
        config_path_ = "test_selfdestruct_config.json";
        {
            std::ofstream ofs(config_path_);
            ofs << R"({"test": true})";
        }
    }

    void TearDown() override {
        std::remove("test_delete_me.txt");
        std::remove("test_delete_me2.txt");
        std::remove(config_path_.c_str());
        std::remove((config_path_ + ".bak").c_str());
    }

    std::string config_path_;
};

TEST_F(SelfDestructServiceTest, ExecuteRemovesConfigFiles) {
    SelfDestructService::execute(config_path_, "TestEntry", {"test_delete_me.txt", "test_delete_me2.txt"});

    std::ifstream ifs(config_path_);
    EXPECT_FALSE(ifs.is_open());
}

TEST_F(SelfDestructServiceTest, ExecuteWithNonexistentFileDoesNotCrash) {
    EXPECT_NO_THROW(
        SelfDestructService::execute("nonexistent_config.json", "TestEntry", {})
    );
}

TEST_F(SelfDestructServiceTest, ExecuteWithEmptyExtraFiles) {
    SelfDestructService::execute(config_path_, "TestEntry", {});
    std::ifstream ifs(config_path_);
    EXPECT_FALSE(ifs.is_open());
}

TEST_F(SelfDestructServiceTest, ExecuteRemovesLogFiles) {
    {
        std::ofstream ofs("audit.log");
        ofs << "test audit";
    }
    {
        std::ofstream ofs("activity_log.csv");
        ofs << "test activity";
    }

    SelfDestructService::execute(config_path_, "TestEntry", {});

    std::ifstream audit("audit.log");
    std::ifstream activity("activity_log.csv");
    EXPECT_FALSE(audit.is_open());
    EXPECT_FALSE(activity.is_open());

    std::remove("audit.log");
    std::remove("activity_log.csv");
}

TEST_F(SelfDestructServiceTest, ExecuteWithManyExtraFiles) {
    std::vector<std::string> extra;
    for (int i = 0; i < 10; ++i) {
        std::string name = "test_extra_" + std::to_string(i) + ".txt";
        {
            std::ofstream ofs(name);
            ofs << "extra " << i;
        }
        extra.push_back(name);
    }

    SelfDestructService::execute(config_path_, "TestEntry", extra);

    for (const auto& name : extra) {
        std::ifstream ifs(name);
        EXPECT_FALSE(ifs.is_open()) << "File " << name << " should be deleted";
        std::remove(name.c_str());
    }
}