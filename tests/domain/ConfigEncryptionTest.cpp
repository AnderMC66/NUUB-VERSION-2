#include <gtest/gtest.h>
#include <fstream>

#include "domain/common/ConfigEncryption.hpp"

using namespace nuub::domain;

class ConfigEncryptionTest : public ::testing::Test {
protected:
    ConfigEncryption enc{"test-password-argon2id"};
};

TEST_F(ConfigEncryptionTest, EncryptDecryptRoundtrip) {
    std::string plaintext = R"({"telegram_bot_token": "TEST", "admin_chat_id": 123})";
    auto encrypted = enc.encrypt(plaintext);
    auto decrypted = enc.decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

TEST_F(ConfigEncryptionTest, EncryptedHasMagicHeader) {
    auto encrypted = enc.encrypt("test data");
    EXPECT_EQ(encrypted[0], 'N');
    EXPECT_EQ(encrypted[1], 'U');
    EXPECT_EQ(encrypted[2], 'C');
    EXPECT_EQ(encrypted[3], 'F');
}

TEST_F(ConfigEncryptionTest, DifferentPlaintextProducesDifferentCiphertext) {
    auto enc1 = enc.encrypt("data1");
    auto enc2 = enc.encrypt("data2");
    EXPECT_NE(enc1, enc2);
}

TEST_F(ConfigEncryptionTest, EmptyPasswordThrows) {
    EXPECT_THROW(ConfigEncryption(""), std::runtime_error);
}

TEST_F(ConfigEncryptionTest, WrongPasswordFails) {
    ConfigEncryption wrong("wrong-password");
    auto encrypted = enc.encrypt("secret data");
    EXPECT_THROW(wrong.decrypt(encrypted), std::runtime_error);
}

TEST_F(ConfigEncryptionTest, TamperedDataFails) {
    auto encrypted = enc.encrypt("important data");
    encrypted[10] ^= 0xFF;
    EXPECT_THROW(enc.decrypt(encrypted), std::runtime_error);
}

TEST_F(ConfigEncryptionTest, InvalidMagicFails) {
    std::vector<uint8_t> bad = {0, 0, 0, 0, 1, 2, 3};
    EXPECT_THROW(enc.decrypt(bad), std::runtime_error);
}

TEST_F(ConfigEncryptionTest, EmptyEncryptedDataFails) {
    std::vector<uint8_t> empty;
    EXPECT_THROW(enc.decrypt(empty), std::runtime_error);
}

TEST_F(ConfigEncryptionTest, SaveAndLoad) {
    std::string json = R"({"key": "value", "num": 42})";
    std::string path = "test_config.nucf";
    EXPECT_TRUE(enc.save(path, json));

    auto loaded = enc.load(path);
    EXPECT_EQ(loaded, json);
    std::remove(path.c_str());
}

TEST_F(ConfigEncryptionTest, LoadNonexistentFile) {
    auto result = enc.load("nonexistent_file.nucf");
    EXPECT_TRUE(result.empty());
}

TEST_F(ConfigEncryptionTest, LoadPlaintextJson) {
    std::string path = "test_plaintext.json";
    std::string content = R"({"plain": true})";
    {
        std::ofstream ofs(path);
        ofs << content;
    }
    auto loaded = enc.load(path);
    EXPECT_EQ(loaded, content);
    std::remove(path.c_str());
}

TEST_F(ConfigEncryptionTest, DifferentSaltsDifferentCiphertext) {
    auto e1 = enc.encrypt("same data");
    auto e2 = enc.encrypt("same data");
    EXPECT_NE(e1, e2);
}

TEST_F(ConfigEncryptionTest, CopiedEncryptionObjectNotAllowed) {
    EXPECT_FALSE(std::is_copy_constructible_v<ConfigEncryption>);
    EXPECT_FALSE(std::is_copy_assignable_v<ConfigEncryption>);
}

TEST_F(ConfigEncryptionTest, LargeConfigRoundtrip) {
    std::string large(100000, 'A');
    auto encrypted = enc.encrypt(large);
    auto decrypted = enc.decrypt(encrypted);
    EXPECT_EQ(decrypted, large);
}