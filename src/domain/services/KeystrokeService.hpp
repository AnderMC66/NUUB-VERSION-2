#pragma once

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
    std::mutex mutex_;
    bool paused_ = false;

    void check_keywords(const std::string& key);

public:
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
};

} // namespace nuub::domain::services
