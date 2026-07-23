#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"
#include "application/interfaces/IFileManagerService.hpp"

namespace nuub::application::commands {

class FileManagerHandler {
    interfaces::IFileManagerService& filemgr_;
    interfaces::IReporter& reporter_;
    std::string pc_id_;

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    FileManagerHandler(
        interfaces::IFileManagerService& filemgr,
        interfaces::IReporter& reporter,
        std::string pc_id);

    domain::Result<void> handle_ls(const std::string& target, const std::string& path);
    domain::Result<void> handle_mkdir(const std::string& target, const std::string& path);
    domain::Result<void> handle_rm(const std::string& target, const std::string& path);
    domain::Result<void> handle_cat(const std::string& target, const std::string& path);
    domain::Result<void> handle_send(const std::string& target, const std::string& path);
};

} // namespace nuub::application::commands
