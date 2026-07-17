#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace nuub::domain::obfuscate {

// Compile-time string encryption using XOR
template <std::size_t N, uint8_t Key>
class EncryptedString {
    std::array<char, N> data_{};
    mutable bool decrypted_ = false;

public:
    // Encrypt at compile time
    constexpr EncryptedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] = str[i] ^ static_cast<char>(Key + i);
        }
    }

    // Decrypt at runtime (one-time)
    const char* decrypt() const {
        if (!decrypted_) {
            for (std::size_t i = 0; i < N; ++i) {
                data_[i] = data_[i] ^ static_cast<char>(Key + i);
            }
            decrypted_ = true;
        }
        return data_.data();
    }

    std::string str() const {
        return std::string(decrypt());
    }

    // Get raw encrypted data (for passing to functions before decryption)
    const char* encrypted_data() const { return data_.data(); }
    constexpr std::size_t size() const { return N; }
};

// Convenience macro for encrypted strings
#define OBFUSCATE(str) ([]() -> const std::string& { \
    static const ::nuub::domain::obfuscate::EncryptedString<sizeof(str), 0x5A> enc(str); \
    static const std::string dec = enc.str(); \
    return dec; \
}())

// Simple XOR encryption for runtime strings
class StringEncryptor {
    static constexpr uint8_t KEY = 0xAB;

public:
    static std::string encrypt(const std::string& input) {
        std::string result = input;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= static_cast<char>(KEY + (i & 0xFF));
        }
        return result;
    }

    static std::string decrypt(const std::string& input) {
        std::string result = input;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= static_cast<char>(KEY + (i & 0xFF));
        }
        return result;
    }
};

} // namespace nuub::domain::obfuscate
