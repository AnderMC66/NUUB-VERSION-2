#pragma once

#include <string>

#include "application/interfaces/IFileManagerService.hpp"

namespace nuub::infrastructure::system {

class WindowsFileManagerService final : public application::interfaces::IFileManagerService {
public:
    [[nodiscard]] std::string list_directory(const std::string& path) override;
    [[nodiscard]] bool create_directory(const std::string& path) override;
    [[nodiscard]] bool delete_file(const std::string& path) override;
    [[nodiscard]] bool delete_directory(const std::string& path) override;
    [[nodiscard]] std::optional<std::string> read_file(const std::string& path) override;
    [[nodiscard]] bool write_file(const std::string& path, const std::string& content) override;
    [[nodiscard]] bool file_exists(const std::string& path) override;
};

} // namespace nuub::infrastructure::system
