#pragma once

#include <string>
#include <optional>
#include "application/interfaces/IFileManagerService.hpp"

namespace nuub::tests::mocks {

class MockFileManagerService final : public application::interfaces::IFileManagerService {
public:
    std::string listing = "file1.txt\nfile2.exe\ndir1/";
    bool create_result = true;
    bool delete_result = true;
    std::string file_content = "file content here";
    bool exists_result = true;

    std::string list_directory(const std::string&) override { return listing; }
    bool create_directory(const std::string&) override { return create_result; }
    bool delete_file(const std::string&) override { return delete_result; }
    bool delete_directory(const std::string&) override { return delete_result; }
    std::optional<std::string> read_file(const std::string&) override {
        return exists_result ? std::optional<std::string>(file_content) : std::nullopt;
    }
    bool write_file(const std::string&, const std::string&) override { return true; }
    bool file_exists(const std::string&) override { return exists_result; }
};

} // namespace nuub::tests::mocks
