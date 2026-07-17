#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

namespace nuub::domain {

class EncryptedChannel {
    std::vector<uint8_t> key_;
    std::vector<uint8_t> hmac_key_;

public:
    EncryptedChannel(const std::string& shared_secret) {
        // Derive encryption key and HMAC key from shared secret
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(shared_secret.c_str()),
               shared_secret.size(), hash);

        key_.assign(hash, hash + 32);

        // Derive HMAC key from different prefix
        std::string hmac_prefix = "HMAC_" + shared_secret;
        SHA256(reinterpret_cast<const unsigned char*>(hmac_prefix.c_str()),
               hmac_prefix.size(), hash);
        hmac_key_.assign(hash, hash + 32);
    }

    // Encrypt data with AES-256-GCM
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) {
        // Generate random nonce
        std::vector<unsigned char> nonce(12);
        RAND_bytes(nonce.data(), 12);

        // Encrypt
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        std::vector<unsigned char> ciphertext(plaintext.size() + 16); // +16 for tag
        int out_len = 0;
        int total_len = 0;

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                               plaintext.data(), plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        total_len = out_len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        total_len += out_len;

        // Get tag
        std::vector<unsigned char> tag(16);
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        EVP_CIPHER_CTX_free(ctx);

        // Build result: nonce(12) + ciphertext(N) + tag(16)
        std::vector<uint8_t> result;
        result.reserve(12 + total_len + 16);
        result.insert(result.end(), nonce.begin(), nonce.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + total_len);
        result.insert(result.end(), tag.begin(), tag.end());

        return result;
    }

    // Decrypt data
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& encrypted) {
        if (encrypted.size() < 12 + 16) return {}; // nonce + tag minimum

        const unsigned char* nonce = encrypted.data();
        const unsigned char* ciphertext = encrypted.data() + 12;
        size_t ciphertext_len = encrypted.size() - 12 - 16;
        const unsigned char* tag = encrypted.data() + encrypted.size() - 16;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        std::vector<unsigned char> plaintext(ciphertext_len);
        int out_len = 0;
        int total_len = 0;

        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, nullptr) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_.data(), nonce) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                               ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }
        total_len = out_len;

        // Set expected tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                                const_cast<unsigned char*>(tag)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {};
        }

        // Final decrypt (verifies tag)
        int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + total_len, &out_len);
        if (ret != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return {}; // Authentication failed
        }
        total_len += out_len;

        EVP_CIPHER_CTX_free(ctx);
        plaintext.resize(total_len);

        return plaintext;
    }

    // Compute HMAC for verification
    std::vector<uint8_t> compute_hmac(const std::vector<uint8_t>& data) {
        unsigned int hmac_len = 32;
        std::vector<unsigned char> hmac(32);

        HMAC(EVP_sha256(),
             hmac_key_.data(), hmac_key_.size(),
             data.data(), data.size(),
             hmac.data(), &hmac_len);

        return std::vector<uint8_t>(hmac.begin(), hmac.begin() + hmac_len);
    }

    // Verify HMAC
    bool verify_hmac(const std::vector<uint8_t>& data, const std::vector<uint8_t>& expected_hmac) {
        auto computed = compute_hmac(data);
        if (computed.size() != expected_hmac.size()) return false;
        return CRYPTO_memcmp(computed.data(), expected_hmac.data(), computed.size()) == 0;
    }
};

} // namespace nuub::domain
