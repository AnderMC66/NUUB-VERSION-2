// NUUB Packer - Build-time executable encryption
// Compile: cl /EHsc /std:c++17 packer.cpp /Fe:packer.exe
// Usage: packer.exe <input.exe> <password>

#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <algorithm>

// XTEA cipher
void xtea_encrypt(uint32_t v[2], const uint32_t key[4]) {
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

// XOR with rolling key
void xor_encrypt(std::vector<uint8_t>& data, const uint8_t* key, size_t key_len) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key_len];
    }
}

// S-box substitution
void substitute(std::vector<uint8_t>& data, uint8_t sbox[256]) {
    for (auto& byte : data) {
        byte = sbox[byte];
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.exe> <password>" << std::endl;
        return 1;
    }

    // Read input executable
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Error: Cannot open " << argv[1] << std::endl;
        return 1;
    }

    std::vector<uint8_t> exe_data(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    input.close();

    std::cout << "Original size: " << exe_data.size() << " bytes" << std::endl;

    // Generate random key from password
    std::string password = argv[2];
    uint8_t key[32]{};
    for (size_t i = 0; i < password.size() && i < 32; ++i) {
        key[i] = static_cast<uint8_t>(password[i]);
    }
    for (int i = 0; i < 32; ++i) {
        key[i] ^= static_cast<uint8_t>(i * 0x37);
    }

    // Generate S-box
    uint8_t sbox[256];
    for (int i = 0; i < 256; ++i) sbox[i] = static_cast<uint8_t>(i);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(std::begin(sbox), std::end(sbox), gen);

    // Apply mutations
    substitute(exe_data, sbox);           // S-box substitution
    xor_encrypt(exe_data, key, 32);       // XOR encryption

    // Add header: magic(8) + sbox(256) + original_size(4) + encrypted_data
    std::vector<uint8_t> output;

    // Magic: "NUUBPACK"
    const char* magic = "NUUBPACK";
    output.insert(output.end(), magic, magic + 8);

    // S-box
    output.insert(output.end(), sbox, sbox + 256);

    // Original size (little-endian)
    uint32_t orig_size = static_cast<uint32_t>(exe_data.size());
    output.push_back(static_cast<uint8_t>(orig_size & 0xFF));
    output.push_back(static_cast<uint8_t>((orig_size >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>((orig_size >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((orig_size >> 24) & 0xFF));

    // Encrypted data
    output.insert(output.end(), exe_data.begin(), exe_data.end());

    // Write output
    std::string output_path = std::string(argv[1]) + ".packed";
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Cannot create " << output_path << std::endl;
        return 1;
    }

    out.write(reinterpret_cast<const char*>(output.data()), output.size());
    out.close();

    std::cout << "Packed size: " << output.size() << " bytes" << std::endl;
    std::cout << "Output: " << output_path << std::endl;
    std::cout << "Ratio: " << (100.0 * output.size() / exe_data.size()) << "%" << std::endl;

    return 0;
}
