#include "domain/services/EncryptionService.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

#include <argon2.h>

namespace nuub::domain::services {

namespace {

void secure_wipe(std::vector<unsigned char>& v) noexcept {
    OPENSSL_cleanse(v.data(), v.size());
    v.clear();
    v.shrink_to_fit();
}

} // anonymous namespace

EncryptionService::EncryptionService(std::string password)
    : password_(std::move(password)) {}

std::vector<unsigned char> EncryptionService::generate_salt() const {
    std::vector<unsigned char> salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }
    return salt;
}

std::vector<unsigned char> EncryptionService::derive_key(
    const unsigned char* salt, int salt_len) const
{
    std::vector<unsigned char> key(KEY_LENGTH);

    int result = argon2id_hash_raw(
        ARGON2_TIME_COST,
        ARGON2_MEMORY_COST,
        ARGON2_PARALLELISM,
        password_.c_str(), password_.size(),
        salt, salt_len,
        key.data(), KEY_LENGTH
    );

    if (result != ARGON2_OK) {
        throw std::runtime_error("Argon2id key derivation failed");
    }

    return key;
}

std::vector<unsigned char> EncryptionService::encrypt(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& aad) const
{
    // Generate random salt and nonce
    auto salt = generate_salt();

    std::vector<unsigned char> nonce(NONCE_SIZE);
    if (RAND_bytes(nonce.data(), NONCE_SIZE) != 1) {
        throw std::runtime_error("Failed to generate random nonce");
    }

    // Derive key using Argon2id
    auto key = derive_key(salt.data(), SALT_SIZE);

    // AES-256-GCM encrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    std::vector<unsigned char> ciphertext(data.size() + EVP_MAX_BLOCK_LENGTH);
    int out_len = 0;
    int total_len = 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM encrypt init failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM IV length");
    }

    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM encrypt init key/iv failed");
    }

    // Add AAD if provided
    if (!aad.empty()) {
        if (EVP_EncryptUpdate(ctx, nullptr, &out_len,
                              aad.data(), static_cast<int>(aad.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to add AAD");
        }
    }

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                          data.data(), static_cast<int>(data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM encrypt update failed");
    }
    total_len = out_len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total_len, &out_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM encrypt final failed");
    }
    total_len += out_len;
    ciphertext.resize(total_len);

    // Get authentication tag
    std::vector<unsigned char> tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get GCM tag");
    }

    EVP_CIPHER_CTX_free(ctx);

    // Clear sensitive data
    secure_wipe(key);

    // Build result: version(1) + key_id(4) + salt(32) + nonce(12) + ciphertext(N) + tag(16)
    std::vector<unsigned char> result;
    result.reserve(HEADER_SIZE + ciphertext.size() + TAG_SIZE);

    // Version byte (0x02 for new format)
    result.push_back(0x02);

    // Key ID (4 bytes, big-endian)
    result.push_back(static_cast<unsigned char>((key_id_ >> 24) & 0xFF));
    result.push_back(static_cast<unsigned char>((key_id_ >> 16) & 0xFF));
    result.push_back(static_cast<unsigned char>((key_id_ >> 8) & 0xFF));
    result.push_back(static_cast<unsigned char>(key_id_ & 0xFF));

    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());
    result.insert(result.end(), tag.begin(), tag.end());

    return result;
}

std::vector<unsigned char> EncryptionService::decrypt(
    const std::vector<unsigned char>& encrypted,
    const std::vector<unsigned char>& aad) const
{
    constexpr int MIN_SIZE = HEADER_SIZE + TAG_SIZE; // header + tag (empty ciphertext allowed)
    if (encrypted.size() < MIN_SIZE) {
        throw std::runtime_error("Encrypted data too short");
    }

    // Parse header
    const unsigned char* ptr = encrypted.data();

    // Version byte
    unsigned char version = *ptr++;

    // Support legacy format (version 0x01 or no version marker)
    if (version != 0x02) {
        throw std::runtime_error("Unsupported encryption format. Migrate data to new format.");
    }

    // Key ID (4 bytes, big-endian)
    std::uint32_t key_id = (static_cast<std::uint32_t>(ptr[0]) << 24) |
                           (static_cast<std::uint32_t>(ptr[1]) << 16) |
                           (static_cast<std::uint32_t>(ptr[2]) << 8) |
                           static_cast<std::uint32_t>(ptr[3]);
    ptr += 4;

    if (key_id != key_id_) {
        throw std::runtime_error("Key ID mismatch - data encrypted with different key");
    }

    // Salt
    const unsigned char* salt = ptr;
    ptr += SALT_SIZE;

    // Nonce
    const unsigned char* nonce = ptr;
    ptr += NONCE_SIZE;

    // Ciphertext and tag
    const unsigned char* ciphertext = ptr;
    size_t ciphertext_len = encrypted.size() - HEADER_SIZE - TAG_SIZE;
    const unsigned char* tag = encrypted.data() + encrypted.size() - TAG_SIZE;

    // Derive key
    auto key = derive_key(salt, SALT_SIZE);

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

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_SIZE, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM IV length");
    }

    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM decrypt init key/iv failed");
    }

    // Add AAD if provided
    if (!aad.empty()) {
        if (EVP_DecryptUpdate(ctx, nullptr, &out_len,
                              aad.data(), static_cast<int>(aad.size())) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to add AAD");
        }
    }

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                          ciphertext, static_cast<int>(ciphertext_len)) != 1) {
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
        secure_wipe(plaintext);
        throw std::runtime_error("GCM authentication failed - bad password, corrupted data, or wrong AAD");
    }
    total_len += out_len;
    plaintext.resize(total_len);

    EVP_CIPHER_CTX_free(ctx);

    // Clear sensitive data
    secure_wipe(key);

    return plaintext;
}

std::uint32_t EncryptionService::get_current_key_id() const {
    return key_id_;
}

void EncryptionService::rotate_key() {
    ++key_id_;
}

} // namespace nuub::domain::services
