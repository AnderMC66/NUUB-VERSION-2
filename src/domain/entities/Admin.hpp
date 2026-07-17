#pragma once

#include <cstdint>
#include <vector>

namespace nuub::domain::entities {

class Admin {
    std::vector<std::int64_t> chat_ids_;

public:
    explicit Admin(std::int64_t chat_id) : chat_ids_{chat_id} {}
    explicit Admin(std::vector<std::int64_t> chat_ids) : chat_ids_(std::move(chat_ids)) {}

    [[nodiscard]] bool is_authorized(std::int64_t chat_id) const {
        for (auto id : chat_ids_) {
            if (id == chat_id) return true;
        }
        return false;
    }

    [[nodiscard]] const std::vector<std::int64_t>& chat_ids() const { return chat_ids_; }

    [[nodiscard]] std::int64_t primary_chat_id() const {
        return chat_ids_.empty() ? 0 : chat_ids_.front();
    }
};

} // namespace nuub::domain::entities
