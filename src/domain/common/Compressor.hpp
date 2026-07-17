#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <zlib.h>

namespace nuub::domain {

class Compressor {
public:
    static bool compress_file(const std::string& input_path, const std::string& output_path) {
        std::ifstream in(input_path, std::ios::binary);
        if (!in.is_open()) return false;

        std::vector<unsigned char> data(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());
        in.close();

        uLongf compressed_size = compressBound(data.size());
        std::vector<unsigned char> compressed(compressed_size);

        int ret = compress2(compressed.data(), &compressed_size,
                           data.data(), data.size(), Z_BEST_COMPRESSION);

        if (ret != Z_OK) return false;

        std::ofstream out(output_path, std::ios::binary);
        if (!out.is_open()) return false;

        // Write original size (8 bytes) then compressed data
        uint64_t orig_size = data.size();
        out.write(reinterpret_cast<const char*>(&orig_size), sizeof(orig_size));
        out.write(reinterpret_cast<const char*>(compressed.data()), compressed_size);
        out.close();

        return true;
    }

    static bool decompress_file(const std::string& input_path, const std::string& output_path) {
        std::ifstream in(input_path, std::ios::binary);
        if (!in.is_open()) return false;

        uint64_t orig_size = 0;
        in.read(reinterpret_cast<char*>(&orig_size), sizeof(orig_size));

        std::vector<unsigned char> compressed(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());
        in.close();

        std::vector<unsigned char> decompressed(orig_size);
        uLongf dest_size = orig_size;

        int ret = uncompress(decompressed.data(), &dest_size,
                            compressed.data(), compressed.size());

        if (ret != Z_OK) return false;

        std::ofstream out(output_path, std::ios::binary);
        if (!out.is_open()) return false;

        out.write(reinterpret_cast<const char*>(decompressed.data()), dest_size);
        out.close();

        return true;
    }

    static std::string compressed_path(const std::string& path) {
        return path + ".gz";
    }
};

} // namespace nuub::domain
