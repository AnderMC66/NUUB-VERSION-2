#include <gtest/gtest.h>
#include <fstream>

#include "domain/common/Compressor.hpp"

using namespace nuub::domain;

class CompressorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test input file
        std::ofstream ofs(input_path_);
        for (int i = 0; i < 10000; ++i) {
            ofs << "Hello, World! This is test data for compression. ";
        }
        ofs.close();
    }

    void TearDown() override {
        std::remove(input_path_.c_str());
        std::remove(compressed_path_.c_str());
        std::remove(decompressed_path_.c_str());
    }

    std::string input_path_ = "test_compressor_input.txt";
    std::string compressed_path_ = Compressor::compressed_path(input_path_);
    std::string decompressed_path_ = "test_compressor_output.txt";
};

TEST_F(CompressorTest, CompressCreatedOutputFile) {
    EXPECT_TRUE(Compressor::compress_file(input_path_, compressed_path_));

    std::ifstream ifs(compressed_path_);
    EXPECT_TRUE(ifs.is_open());
    ifs.close();
}

TEST_F(CompressorTest, CompressedFileIsSmaller) {
    ASSERT_TRUE(Compressor::compress_file(input_path_, compressed_path_));

    std::ifstream in_orig(input_path_, std::ios::binary | std::ios::ate);
    std::ifstream in_comp(compressed_path_, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(in_orig.is_open());
    ASSERT_TRUE(in_comp.is_open());

    auto orig_size = in_orig.tellg();
    auto comp_size = in_comp.tellg();

    EXPECT_LT(comp_size, orig_size);
}

TEST_F(CompressorTest, CompressAndDecompressRoundtrip) {
    ASSERT_TRUE(Compressor::compress_file(input_path_, compressed_path_));
    ASSERT_TRUE(Compressor::decompress_file(compressed_path_, decompressed_path_));

    std::ifstream orig(input_path_);
    std::ifstream decomp(decompressed_path_);
    ASSERT_TRUE(orig.is_open());
    ASSERT_TRUE(decomp.is_open());

    std::string orig_content((std::istreambuf_iterator<char>(orig)),
                              std::istreambuf_iterator<char>());
    std::string decomp_content((std::istreambuf_iterator<char>(decomp)),
                                std::istreambuf_iterator<char>());

    EXPECT_EQ(orig_content, decomp_content);
}

TEST_F(CompressorTest, CompressNonexistentFile) {
    EXPECT_FALSE(Compressor::compress_file("nonexistent.txt", "out.gz"));
}

TEST_F(CompressorTest, DecompressNonexistentFile) {
    EXPECT_FALSE(Compressor::decompress_file("nonexistent.gz", "out.txt"));
}

TEST_F(CompressorTest, CompressedPathAppendsGz) {
    EXPECT_EQ(Compressor::compressed_path("test.txt"), "test.txt.gz");
    EXPECT_EQ(Compressor::compressed_path("/path/to/file.bin"), "/path/to/file.bin.gz");
}

TEST_F(CompressorTest, SmallFileCompression) {
    std::string small_path = "test_small.txt";
    {
        std::ofstream ofs(small_path);
        ofs << "tiny";
    }
    EXPECT_TRUE(Compressor::compress_file(small_path, "test_small.txt.gz"));
    EXPECT_TRUE(Compressor::decompress_file("test_small.txt.gz", "test_small_out.txt"));
    std::remove(small_path.c_str());
    std::remove("test_small.txt.gz");
    std::remove("test_small_out.txt");
}

TEST_F(CompressorTest, EmptyFileCompression) {
    std::string empty_path = "test_empty.txt";
    {
        std::ofstream ofs(empty_path);
    }
    EXPECT_TRUE(Compressor::compress_file(empty_path, "test_empty.txt.gz"));
    EXPECT_TRUE(Compressor::decompress_file("test_empty.txt.gz", "test_empty_out.txt"));
    std::remove(empty_path.c_str());
    std::remove("test_empty.txt.gz");
    std::remove("test_empty_out.txt");
}