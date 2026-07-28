#include <gtest/gtest.h>

#include "domain/common/Result.hpp"

using namespace nuub::domain;

TEST(ResultTest, ValueSuccess) {
    auto r = Result<int>::success(42);
    EXPECT_TRUE(r.is_success());
    EXPECT_FALSE(r.is_failure());
    EXPECT_EQ(r.value(), 42);
}

TEST(ResultTest, ValueFailure) {
    auto r = Result<int>::failure("something went wrong");
    EXPECT_FALSE(r.is_success());
    EXPECT_TRUE(r.is_failure());
    EXPECT_EQ(r.error(), "something went wrong");
}

TEST(ResultTest, ValueStringSuccess) {
    auto r = Result<std::string>::success("hello");
    EXPECT_TRUE(r.is_success());
    EXPECT_EQ(r.value(), "hello");
}

TEST(ResultTest, ValueStringFailure) {
    auto r = Result<std::string>::failure("error");
    EXPECT_TRUE(r.is_failure());
    EXPECT_EQ(r.error(), "error");
}

TEST(ResultTest, VoidSuccess) {
    auto r = Result<void>::success();
    EXPECT_TRUE(r.is_success());
    EXPECT_FALSE(r.is_failure());
}

TEST(ResultTest, VoidFailure) {
    auto r = Result<void>::failure("void error");
    EXPECT_FALSE(r.is_success());
    EXPECT_TRUE(r.is_failure());
    EXPECT_EQ(r.error(), "void error");
}

TEST(ResultTest, MultipleSuccesses) {
    auto r1 = Result<int>::success(1);
    auto r2 = Result<int>::success(2);
    auto r3 = Result<int>::success(3);
    EXPECT_TRUE(r1.is_success());
    EXPECT_TRUE(r2.is_success());
    EXPECT_TRUE(r3.is_success());
    EXPECT_EQ(r1.value(), 1);
    EXPECT_EQ(r2.value(), 2);
    EXPECT_EQ(r3.value(), 3);
}

TEST(ResultTest, EmptyErrorString) {
    auto r = Result<void>::failure("");
    EXPECT_TRUE(r.is_failure());
    EXPECT_TRUE(r.error().empty());
}

TEST(ResultTest, CustomTypeSuccess) {
    struct Point { int x, y; };
    auto r = Result<Point>::success({10, 20});
    EXPECT_TRUE(r.is_success());
    EXPECT_EQ(r.value().x, 10);
    EXPECT_EQ(r.value().y, 20);
}