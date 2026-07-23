#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace nuub::domain {

// Encrypt/decrypt config file at rest using AES-256-GCM
// The config is encrypted with a key derived from a machine-specific seed
class ConfigEncryption {
    std::vector<uint8_t> key_;

    // Derive key from password using SHA-256
    static std::vector<uint8_t> derive_key(const std::string& password) {
        // Use password as-is for key derivation (cross-platform)
        // In production, this should use Argon2id like EncryptionService
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()),
               password.size(), hash);

        return std::vector<uint8_t>(hash, hash + 32);
    }

public:
    explicit ConfigEncryption(const std::string& password)
        : key_(derive_key(password)) {}

    // Encrypt plaintext → encrypted blob
    std::vector<uint8_t> encrypt(const std::string& plaintext) {
        // Generate random IV
        std::vector<unsigned char> iv(12);
        RAND_bytes(iv.data(), 12);

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        std::vector<unsigned char> ciphertext(plaintext.size() + 16);
        int out_len = 0;
        int total_len = 0;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv.data());

        EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                          reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                          plaintext.size());
        total_len = out_len;

        EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len);
        total_len += out_len;

        // Get tag
        std::vector<unsigned char> tag(16);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data());

        EVP_CIPHER_CTX_free(ctx);

        // Build result: magic(4) + iv(12) + ciphertext(N) + tag(16)
        std::vector<uint8_t> result;
        result.reserve(4 + 12 + total_len + 16);

        // Magic marker
        result.push_back('N');
        result.push_back('U');
        result.push_back('C');
        result.push_back('F'); // NUub Config File

        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + total_len);
        result.insert(result.end(), tag.begin(), tag.end());

        return result;
    }

    // Decrypt encrypted blob → plaintext
    std::string decrypt(const std::vector<uint8_t>& encrypted) {
        if (encrypted.size() < 4 + 12 + 16) return {};

        // Verify magic
        if (encrypted[0] != 'N' || encrypted[1] != 'U' ||
            encrypted[2] != 'C' || encrypted[3] != 'F') {
            // Not encrypted — return as-is
            return std::string(encrypted.begin(), encrypted.end());
        }

        const unsigned char* iv = encrypted.data() + 4;
        const unsigned char* ciphertext = encrypted.data() + 16;
        size_t ciphertext_len = encrypted.size() - 16 - 16 - 4;
        const unsigned char* tag = encrypted.data() + encrypted.size() - 16;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        std::vector<unsigned char> plaintext(ciphertext_len);
        int out_len = 0;
        int total_len = 0;

        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr);
        EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), iv);

        EVP_DecryptUpdate(ctx, plaintext.data(), &out_len, ciphertext, ciphertext_len);
        total_len = out_len;

        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                            const_cast<unsigned char*>(tag));

        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
        EVP_CIPHER_CTX_free(ctx);

        if (ret != 1) return {}; // Authentication failed

        total_len += out_len;
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
