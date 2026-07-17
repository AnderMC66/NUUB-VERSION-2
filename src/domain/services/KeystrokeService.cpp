#include "domain/services/KeystrokeService.hpp"

#include <algorithm>

namespace nuub::domain::services {

void KeystrokeService::check_keywords(const std::string& key) {
    if (keywords_.empty() || !keyword_callback_) return;

    // Build current word from buffer
    std::string word;
    for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
        const auto& k = it->key;
        if (k == " " || k == "\n" || k == "\t") break;
        word = k + word;
    }

    // Check if any keyword is contained in the current word
    std::string lower_word = word;
    std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);

    for (const auto& kw : keywords_) {
        std::string lower_kw = kw;
        std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(), ::tolower);
        if (lower_word.find(lower_kw) != std::string::npos) {
            keyword_callback_("Keyword detected: '" + kw + "' in context: '" + word + "'");
            return;
        }
    }
}

void KeystrokeService::process_press(const std::string& key) {
    if (paused_) return;
    if (key.empty()) return;

    std::lock_guard lock(mutex_);
    buffer_.emplace_back(key);
    check_keywords(key);
}

void KeystrokeService::process_release(const std::string& /*key*/) {
}

void KeystrokeService::pause() { paused_ = true; }
void KeystrokeService::resume() { paused_ = false; }
bool KeystrokeService::is_paused() const { return paused_; }

std::string KeystrokeService::get_log() {
    std::lock_guard lock(mutex_);
    std::string result;
    for (const auto& entry : buffer_) result += entry.key;
    return result;
}

std::string KeystrokeService::clear_log() {
    std::lock_guard lock(mutex_);
    std::string result;
    for (const auto& entry : buffer_) result += entry.key;
    buffer_.clear();
    return result;
}

void KeystrokeService::add_keyword(const std::string& keyword) {
    std::lock_guard lock(mutex_);
    keywords_.insert(keyword);
}

void KeystrokeService::remove_keyword(const std::string& keyword) {
    std::lock_guard lock(mutex_);
    keywords_.erase(keyword);
}

std::vector<std::string> KeystrokeService::get_keywords() const {
    return {keywords_.begin(), keywords_.end()};
}

void KeystrokeService::set_keyword_callback(std::function<void(const std::string&)> callback) {
    keyword_callback_ = std::move(callback);
}

} // namespace nuub::domain::services
