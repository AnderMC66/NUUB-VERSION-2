#pragma once

#include <optional>
#include <string>
#include <utility>

namespace nuub::domain {

template <typename T>
class Result {
    std::optional<T> value_;
    std::string error_;

    Result(std::optional<T> val, std::string err)
        : value_(std::move(val)), error_(std::move(err)) {}

public:
    static Result success(T val) {
        return Result(std::move(val), {});
    }

    static Result failure(std::string err) {
        return Result(std::nullopt, std::move(err));
    }

    [[nodiscard]] bool is_success() const { return value_.has_value(); }
    [[nodiscard]] bool is_failure() const { return !value_.has_value(); }
    [[nodiscard]] const T& value() const { return *value_; }
    [[nodiscard]] const std::string& error() const { return error_; }
};

template <>
class Result<void> {
    std::string error_;
    bool success_;

    explicit Result(bool ok, std::string err)
        : error_(std::move(err)), success_(ok) {}

public:
    static Result success() { return Result(true, {}); }
    static Result failure(std::string err) { return Result(false, std::move(err)); }

    [[nodiscard]] bool is_success() const { return success_; }
    [[nodiscard]] bool is_failure() const { return !success_; }
    [[nodiscard]] const std::string& error() const { return error_; }
};

} // namespace nuub::domain
