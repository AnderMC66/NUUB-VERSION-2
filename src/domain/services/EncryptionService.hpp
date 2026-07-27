#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "domain/services/IEncryptionService.hpp"

namespace nuub::domain::services {

class EncryptionService final : public IEncryptionService {
    std::string password_;
    std::uint32_t key_id_{1};

    static constexpr int ARGON2_TIME_COST = 4;
    static constexpr int ARGON2_MEMORY_COST = 65536; // 64 MB
    static constexpr int ARGON2_PARALLELISM = 4;
    static constexpr int SALT_SIZE = 32;
    static constexpr int KEY_LENGTH = 32;
    static constexpr int NONCE_SIZE = 12; // GCM standard
    static constexpr int TAG_SIZE = 16;   // GCM standard

    // Header layout: version(1) + key_id(4) + salt(32) + nonce(12)
    static constexpr int HEADER_SIZE = 1 + 4 + SALT_SIZE + NONCE_SIZE;

    [[nodiscard]] std::vector<unsigned char> derive_key(
        const unsigned char* salt, int salt_len) const;

    [[nodiscard]] std::vector<unsigned char> generate_salt() const;

public:
    explicit EncryptionService(std::string password);

    ~EncryptionService() {
        // Securely wipe password from memory
        wipe_password();
    }

    // Prevent copy to avoid password duplication
    EncryptionService(const EncryptionService&) = delete;
    EncryptionService& operator=(const EncryptionService&) = delete;

    [[nodiscard]] std::vector<unsigned char> encrypt(
        const std::vector<unsigned char>& data,
        const std::vector<unsigned char>& aad = {}) const override;

    [[nodiscard]] std::vector<unsigned char> decrypt(
        const std::vector<unsigned char>& encrypted,
        const std::vector<unsigned char>& aad = {}) const override;

    [[nodiscard]] std::uint32_t get_current_key_id() const override;

    void rotate_key() override;

private:
    void wipe_password() {
        // Overwrite password memory with zeros
        if (!password_.empty()) {
            volatile char* ptr = const_cast<volatile char*>(password_.data());
            for (size_t i = 0; i < password_.size(); ++i) {
                ptr[i] = 0;
            }
            password_.clear();
        }
    }
};

} // namespace nuub::domain::services
