#pragma once

#include <vector>
#include <cstdint>
#include <random>
#include <algorithm>
#include <cstring>

namespace nuub::domain {

class MutationEngine {
    std::mt19937 rng_;

public:
    MutationEngine() : rng_(std::random_device{}()) {}

    // Generate a random key for this build
    std::vector<uint8_t> generate_key(int length = 32) {
        std::vector<uint8_t> key(length);
        std::uniform_int_distribution<int> dist(0, 255);
        for (auto& byte : key) {
            byte = static_cast<uint8_t>(dist(rng_));
        }
        return key;
    }

    // XOR encrypt/decrypt with rolling key
    std::vector<uint8_t> xor_mutate(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) {
        std::vector<uint8_t> result = data;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= key[i % key.size()];
        }
        return result;
    }

    // Byte substitution (S-box style)
    std::vector<uint8_t> substitute(const std::vector<uint8_t>& data) {
        // Generate random substitution table
        uint8_t sbox[256];
        for (int i = 0; i < 256; ++i) sbox[i] = static_cast<uint8_t>(i);

        std::shuffle(std::begin(sbox), std::end(sbox), rng_);

        std::vector<uint8_t> result(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = sbox[data[i]];
        }
        return result;
    }

    // Reverse substitution
    std::vector<uint8_t> inverse_substitute(const std::vector<uint8_t>& data, const uint8_t sbox[256]) {
        uint8_t inv_sbox[256];
        for (int i = 0; i < 256; ++i) inv_sbox[sbox[i]] = static_cast<uint8_t>(i);

        std::vector<uint8_t> result(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = inv_sbox[data[i]];
        }
        return result;
    }

    // Byte shuffling (preserve content but change order)
    std::vector<uint8_t> shuffle_bytes(const std::vector<uint8_t>& data) {
        std::vector<std::pair<size_t, uint8_t>> indexed(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            indexed[i] = {i, data[i]};
        }
        std::shuffle(indexed.begin(), indexed.end(), rng_);

        std::vector<uint8_t> result(data.size());
        std::vector<size_t> permutation(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = indexed[i].second;
            permutation[indexed[i].first] = i;
        }
        return result;
    }

    // Insert junk bytes
    std::vector<uint8_t> insert_junk(const std::vector<uint8_t>& data, int junk_ratio = 10) {
        std::uniform_int_distribution<int> dist(0, 100);
        std::vector<uint8_t> result;

        for (size_t i = 0; i < data.size(); ++i) {
            result.push_back(data[i]);

            // Randomly insert junk bytes
            if (dist(rng_) < junk_ratio) {
                int junk_count = 1 + (rng_ % 4);
                for (int j = 0; j < junk_count; ++j) {
                    result.push_back(static_cast<uint8_t>(rng_() & 0xFF));
                }
            }
        }
        return result;
    }

    // Full mutation pipeline
    std::vector<uint8_t> mutate(const std::vector<uint8_t>& data) {
        auto key = generate_key();

        // Apply mutations in sequence
        auto result = substitute(data);           // S-box substitution
        result = xor_mutate(result, key);         // XOR with random key
        result = shuffle_bytes(result);           // Shuffle bytes
        result = insert_junk(result, 5);          // Insert junk

        return result;
    }

    // Generate stub code for decryption
    std::vector<uint8_t> generate_stub(const std::vector<uint8_t>& key) {
        // This generates a small stub that decrypts the payload
        // For simplicity, we use XOR decryption
        std::vector<uint8_t> stub;

        // XOR decryption loop stub (x64 shellcode-like)
        // In practice, this would be actual machine code
        stub.push_back(0x48); // REX.W prefix
        stub.push_back(0x89); // MOV RAX, RCX
        stub.push_back(0xC8);

        return stub;
    }
};

} // namespace nuub::domain
