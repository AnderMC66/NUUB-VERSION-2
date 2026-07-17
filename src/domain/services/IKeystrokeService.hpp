#pragma once

#include <functional>
#include <string>
#include <vector>

namespace nuub::domain::services {

class IKeystrokeService {
public:
    virtual ~IKeystrokeService() = default;

    virtual void process_press(const std::string& key) = 0;
    virtual void process_release(const std::string& key) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    [[nodiscard]] virtual bool is_paused() const = 0;
    [[nodiscard]] virtual std::string get_log() = 0;
    virtual std::string clear_log() = 0;

    virtual void add_keyword(const std::string& keyword) = 0;
    virtual void remove_keyword(const std::string& keyword) = 0;
    [[nodiscard]] virtual std::vector<std::string> get_keywords() const = 0;
    virtual void set_keyword_callback(std::function<void(const std::string&)> callback) = 0;
};

} // namespace nuub::domain::services
