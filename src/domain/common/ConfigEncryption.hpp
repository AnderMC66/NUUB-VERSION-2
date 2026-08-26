#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <argon2.h>

namespace nuub::domain {

// Encrypt/decrypt config file at rest using AES-256-GCM with Argon2id KDF
// Format: NUCF(4) + salt(32) + iv(12) + ciphertext(N) + tag(16)
class ConfigEncryption {
    static constexpr int ARGON2_TIME_COST = 4;
    static constexpr int ARGON2_MEMORY_COST = 16384; // 16 MB (reduced from 64 MB)
    static constexpr int ARGON2_PARALLELISM = 4;
    static constexpr int SALT_SIZE = 32;
    static constexpr int KEY_LENGTH = 32;
    static constexpr int IV_SIZE = 12;
    static constexpr int TAG_SIZE = 16;
    static constexpr int MIN_ENCRYPTED_SIZE = 4 + SALT_SIZE + IV_SIZE + TAG_SIZE;

    std::string password_;

    static std::vector<unsigned char> generate_salt() {
        std::vector<unsigned char> salt(SALT_SIZE);
        if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
            throw std::runtime_error("Failed to generate random salt");
        }
        return salt;
    }

    static std::vector<unsigned char> derive_key(
        const std::string& password,
        const unsigned char* salt, int salt_len)
    {
        std::vector<unsigned char> key(KEY_LENGTH);

        int result = argon2id_hash_raw(
            ARGON2_TIME_COST,
            ARGON2_MEMORY_COST,
            ARGON2_PARALLELISM,
            password.c_str(), password.size(),
            salt, salt_len,
            key.data(), KEY_LENGTH
        );

        if (result != ARGON2_OK) {
            throw std::runtime_error("Argon2id key derivation failed for config encryption");
        }

        return key;
    }

public:
    explicit ConfigEncryption(std::string password)
        : password_(std::move(password)) {
        if (this->password_.empty()) {
            throw std::runtime_error("ConfigEncryption password cannot be empty");
        }
    }

    ~ConfigEncryption() {
        // Wipe password from memory
        if (!password_.empty()) {
            volatile char* ptr = const_cast<volatile char*>(password_.data());
            for (size_t i = 0; i < password_.size(); ++i) {
                ptr[i] = 0;
            }
            password_.clear();
        }
    }

    ConfigEncryption(const ConfigEncryption&) = delete;
    ConfigEncryption& operator=(const ConfigEncryption&) = delete;

    // Encrypt plaintext -> encrypted blob
    std::vector<uint8_t> encrypt(const std::string& plaintext) {
        // Generate random salt and IV
        auto salt = generate_salt();

        std::vector<unsigned char> iv(IV_SIZE);
        if (RAND_bytes(iv.data(), IV_SIZE) != 1) {
            throw std::runtime_error("Failed to generate random IV");
        }

        // Derive key with Argon2id
        auto key = derive_key(password_, salt.data(), SALT_SIZE);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
        int out_len = 0;
        int total_len = 0;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM encrypt init failed");
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set GCM IV length");
        }

        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM encrypt init key/iv failed");
        }

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                              reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                              plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM encrypt update failed");
        }
        total_len = out_len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM encrypt final failed");
        }
        total_len += out_len;

        // Get tag
        std::vector<unsigned char> tag(TAG_SIZE);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to get GCM tag");
        }

        EVP_CIPHER_CTX_free(ctx);

        // Clear key from memory
        OPENSSL_cleanse(key.data(), key.size());

        // Build result: NUCF(4) + salt(32) + iv(12) + ciphertext(N) + tag(16)
        std::vector<uint8_t> result;
        result.reserve(4 + SALT_SIZE + IV_SIZE + total_len + TAG_SIZE);

        // Magic marker
        result.push_back('N');
        result.push_back('U');
        result.push_back('C');
        result.push_back('F');

        result.insert(result.end(), salt.begin(), salt.end());
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + total_len);
        result.insert(result.end(), tag.begin(), tag.end());

        return result;
    }

    // Decrypt encrypted blob -> plaintext
    std::string decrypt(const std::vector<uint8_t>& encrypted) {
        if (encrypted.size() < MIN_ENCRYPTED_SIZE) {
            throw std::runtime_error("Encrypted config data too short");
        }

        // Verify magic
        if (encrypted[0] != 'N' || encrypted[1] != 'U' ||
            encrypted[2] != 'C' || encrypted[3] != 'F') {
            throw std::runtime_error("Invalid NUCF magic in config file");
        }

        const unsigned char* ptr = encrypted.data() + 4;

        // Salt
        const unsigned char* salt = ptr;
        ptr += SALT_SIZE;

        // IV
        const unsigned char* iv = ptr;
        ptr += IV_SIZE;

        // Ciphertext and tag
        const unsigned char* ciphertext = ptr;
        size_t ciphertext_len = encrypted.size() - 4 - SALT_SIZE - IV_SIZE - TAG_SIZE;
        const unsigned char* tag = encrypted.data() + encrypted.size() - TAG_SIZE;

        // Derive key with Argon2id
        auto key = derive_key(password_, salt, SALT_SIZE);

        // AES-256-GCM decrypt
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) throw std::runtime_error("Failed to create cipher context");

        std::vector<unsigned char> plaintext(ciphertext_len + EVP_MAX_BLOCK_LENGTH);
        int out_len = 0;
        int total_len = 0;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM decrypt init failed");
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set GCM IV length");
        }

        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM decrypt init key/iv failed");
        }

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                              ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("GCM decrypt update failed");
        }
        total_len = out_len;

        // Set expected tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                                const_cast<unsigned char*>(tag)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to set GCM tag");
        }

        // Final decrypt (verifies tag)
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
        if (ret != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(plaintext.data(), plaintext.size());
            throw std::runtime_error("GCM authentication failed - wrong password or corrupted config");
        }
        total_len += out_len;

        EVP_CIPHER_CTX_free(ctx);

        // Clear key from memory
        OPENSSL_cleanse(key.data(), key.size());

        return std::string(plaintext.begin(), plaintext.begin() + total_len);
    }

    // Save encrypted config to file
    bool save(const std::string& path, const std::string& json_content) {
        auto encrypted = encrypt(json_content);
        if (encrypted.empty()) return false;

        std::ofstream ofs(path, std::ios::binary);
        if (!ofs.is_open()) return false;
        ofs.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        return ofs.good();
    }

    // Load config from file (auto-detect encrypted vs plaintext)
    std::string load(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) return {};

        std::vector<uint8_t> data(
            (std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        if (data.empty()) return {};

        // Check if it's encrypted (starts with NUCF magic)
        if (data.size() >= 4 && data[0] == 'N' && data[1] == 'U' &&
            data[2] == 'C' && data[3] == 'F') {
            return decrypt(data);
        }

        // Not encrypted — return raw content
        return std::string(data.begin(), data.end());
    }
};

} // namespace nuub::domain
