#pragma once

#include <vector>
#include <cstdint>
#include <cstring>

namespace nuub::domain {

class Packer {
    static constexpr uint64_t MAGIC = 0x4E555542205632ULL; // "NUUB V2" in hex

    // Simple XTEA block cipher
    static void xtea_encrypt(uint32_t v[2], const uint32_t key[4]) {
        uint32_t v0 = v[0], v1 = v[1];
        uint32_t sum = 0;
        uint32_t delta = 0x9E3779B9;

        for (int i = 0; i < 32; ++i) {
            v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
            sum += delta;
            v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
        }
        v[0] = v0;
        v[1] = v1;
    }

    static void xtea_decrypt(uint32_t v[2], const uint32_t key[4]) {
        uint32_t v0 = v[0], v1 = v[1];
        uint32_t delta = 0x9E3779B9;
        uint32_t sum = delta * 32;

        for (int i = 0; i < 32; ++i) {
            v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
            sum -= delta;
            v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        }
        v[0] = v0;
        v[1] = v1;
    }

public:
    static std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data, const std::string& password) {
        // Derive key from password
        uint32_t key[4]{};
        for (size_t i = 0; i < password.size(); ++i) {
            key[i % 4] ^= static_cast<uint32_t>(password[i]) << ((i % 4) * 8);
        }
        // Mix key
        key[0] ^= 0x4E555542;
        key[1] ^= 0x20563200;
        key[2] ^= 0xDEADBEEF;
        key[3] ^= 0xCAFEBABE;

        std::vector<uint8_t> result;
        result.resize(sizeof(uint64_t) + data.size());

        // Write magic
        *reinterpret_cast<uint64_t*>(result.data()) = MAGIC;

        // Copy and encrypt data
        memcpy(result.data() + sizeof(uint64_t), data.data(), data.size());

        auto* encrypted = reinterpret_cast<uint32_t*>(result.data() + sizeof(uint64_t));
        size_t num_blocks = data.size() / 8;
        for (size_t i = 0; i < num_blocks; ++i) {
            xtea_encrypt(&encrypted[i * 2], key);
        }

        return result;
    }

    static std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data, const std::string& password) {
        if (data.size() < sizeof(uint64_t)) return {};

        // Check magic
        uint64_t magic = *reinterpret_cast<const uint64_t*>(data.data());
        if (magic != MAGIC) return {};

        // Derive key
        uint32_t key[4]{};
        for (size_t i = 0; i < password.size(); ++i) {
            key[i % 4] ^= static_cast<uint32_t>(password[i]) << ((i % 4) * 8);
        }
        key[0] ^= 0x4E555542;
        key[1] ^= 0x20563200;
        key[2] ^= 0xDEADBEEF;
        key[3] ^= 0xCAFEBABE;

        std::vector<uint8_t> result(data.size() - sizeof(uint64_t));
        memcpy(result.data(), data.data() + sizeof(uint64_t), result.size());

        // Decrypt
        auto* decrypted = reinterpret_cast<uint32_t*>(result.data());
        size_t num_blocks = result.size() / 8;
        for (size_t i = 0; i < num_blocks; ++i) {
            xtea_decrypt(&decrypted[i * 2], key);
        }

        return result;
    }
};

} // namespace nuub::domain
