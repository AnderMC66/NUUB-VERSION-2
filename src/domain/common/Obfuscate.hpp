#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

namespace nuub::domain::obfuscate {

// Compile-time string encryption using rolling XOR
// Key derivation: base key XOR'd with position-based entropy
template <std::size_t N, uint8_t Key>
class EncryptedString {
    std::array<char, N> data_{};
    mutable bool decrypted_ = false;

    // Per-byte rolling key derived from position
    static constexpr char byte_key(std::size_t i) {
        // Mix base key with position using different constants per byte
        uint8_t k = static_cast<uint8_t>(
            (Key ^ (i * 0x37) ^ ((i >> 2) * 0x13) ^ ((i >> 1) * 0x5B)) & 0xFF);
        return static_cast<char>(k);
    }

public:
    constexpr EncryptedString(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i) {
            data_[i] = str[i] ^ byte_key(i);
        }
    }

    // Decrypt lazily on first access
    const char* decrypt() const {
        if (!decrypted_) {
            for (std::size_t i = 0; i < N; ++i) {
                data_[i] = data_[i] ^ byte_key(i);
            }
            decrypted_ = true;
        }
        return data_.data();
    }

    std::string str() const {
        return std::string(decrypt());
    }

    const char* encrypted_data() const { return data_.data(); }
    constexpr std::size_t size() const { return N; }
};

// Compile-time integer encryption
template <uint32_t Value, uint8_t Key>
struct EncryptedInt {
    static constexpr uint32_t decrypt() {
        // XOR with key-derived value
        uint32_t k = static_cast<uint32_t>(Key) * 0x01000193;
        return Value ^ k;
    }
};

// Compile-time pointer encryption (for function pointers)
template <uintptr_t Value, uint8_t Key>
struct EncryptedPtr {
    static constexpr uintptr_t decrypt() {
        uintptr_t k = static_cast<uintptr_t>(Key) * 0x01000193;
        return Value ^ k;
    }
};

// ── OBFUSCATE macro ────────────────────────────────────────────
// Uses __COUNTER__ for unique keys per call site
// Each invocation gets a different encryption key
#define OBFUSCATE(str) ([]() -> const std::string& { \
    static const ::nuub::domain::obfuscate::EncryptedString<sizeof(str), \
        static_cast<uint8_t>((__COUNTER__ + __LINE__ + 0x5A) & 0xFF)> enc(str); \
    static const std::string dec = enc.str(); \
    return dec; \
}())

// OBFUSCATE_A returns char* directly (no string allocation)
#define OBFUSCATE_A(str) ([]() -> const char* { \
    static const ::nuub::domain::obfuscate::EncryptedString<sizeof(str), \
        static_cast<uint8_t>((__COUNTER__ + __LINE__ + 0x37) & 0xFF)> enc(str); \
    return enc.decrypt(); \
}())

// OBFUSCATE_INT encrypts a compile-time integer
#define OBFUSCATE_INT(val) \
    ::nuub::domain::obfuscate::EncryptedInt<val, \
        static_cast<uint8_t>((__COUNTER__ + __LINE__) & 0xFF)>::decrypt()

// ── Runtime String Encryptor ────────────────────────────────────
class StringEncryptor {
public:
    // Encrypt with rolling XOR derived from input characteristics
    static std::string encrypt(const std::string& input) {
        if (input.empty()) return {};

        // Derive unique key from input content + length
        uint8_t seed = 0;
        for (char c : input) seed ^= static_cast<uint8_t>(c);
        seed = static_cast<uint8_t>((seed + input.size() * 0x37) & 0xFF);
        if (seed == 0) seed = 0xAB;

        std::string result = input;
        for (size_t i = 0; i < result.size(); ++i) {
            uint8_t k = static_cast<uint8_t>(
                (seed ^ (i * 0x13) ^ ((i >> 3) * 0x71) ^ ((i >> 1) * 0x29)) & 0xFF);
            result[i] ^= k;
        }
        return result;
    }

    // Decrypt (same operation — rolling XOR is symmetric)
    static std::string decrypt(const std::string& input) {
        return encrypt(input);
    }
};

// ── Compile-time hash for API resolution ────────────────────────
// FNV-1a hash usable at compile time
constexpr uint32_t fnv1a(const char* str) {
    uint32_t hash = 0x811C9DC5;
    while (*str) {
        hash ^= static_cast<uint32_t>(*str);
        hash *= 0x01000193;
        str++;
    }
    return hash;
}

// Helper to convert a string literal to encrypted form at compile time
template <std::size_t N>
constexpr auto make_encrypted(const char (&str)[N]) {
    return EncryptedString<N, static_cast<uint8_t>((N * 0x37 + 0x5A) & 0xFF)>(str);
}

} // namespace nuub::domain::obfuscate
