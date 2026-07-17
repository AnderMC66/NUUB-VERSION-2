#pragma once

#include <optional>
#include <string>

namespace nuub::application::interfaces {

class IFileManagerService {
public:
    virtual ~IFileManagerService() = default;

    [[nodiscard]] virtual std::string list_directory(const std::string& path) = 0;
    [[nodiscard]] virtual bool create_directory(const std::string& path) = 0;
    [[nodiscard]] virtual bool delete_file(const std::string& path) = 0;
    [[nodiscard]] virtual bool delete_directory(const std::string& path) = 0;
    [[nodiscard]] virtual std::optional<std::string> read_file(const std::string& path) = 0;
    [[nodiscard]] virtual bool write_file(const std::string& path, const std::string& content) = 0;
    [[nodiscard]] virtual bool file_exists(const std::string& path) = 0;
};

} // namespace nuub::application::interfaces
