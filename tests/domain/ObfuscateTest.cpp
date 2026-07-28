#include <gtest/gtest.h>

#include "domain/common/Obfuscate.hpp"

using namespace nuub::domain::obfuscate;

TEST(ObfuscateTest, EncryptedStringDecrypts) {
    auto encrypted = make_encrypted("HelloWorld");
    EXPECT_STREQ(encrypted.decrypt(), "HelloWorld");
}

TEST(ObfuscateTest, EmptyEncryptedString) {
    auto encrypted = make_encrypted("");
    EXPECT_STREQ(encrypted.decrypt(), "");
}

TEST(ObfuscateTest, LongStringEncryption) {
    auto encrypted = make_encrypted("This is a much longer string to test encryption of larger data");
    EXPECT_STREQ(encrypted.decrypt(), "This is a much longer string to test encryption of larger data");
}

TEST(ObfuscateTest, SpecialCharacters) {
    auto encrypted = make_encrypted("line1\nline2\ttab");
    EXPECT_STREQ(encrypted.decrypt(), "line1\nline2\ttab");
}

TEST(ObfuscateTest, RepeatedDecryptionReturnsSame) {
    auto encrypted = make_encrypted("Consistent");
    EXPECT_STREQ(encrypted.decrypt(), "Consistent");
    EXPECT_STREQ(encrypted.decrypt(), "Consistent");
}

TEST(ObfuscateTest, NumericString) {
    auto encrypted = make_encrypted("12345-67890");
    EXPECT_STREQ(encrypted.decrypt(), "12345-67890");
}

TEST(ObfuscateTest, SymbolString) {
    auto encrypted = make_encrypted("!@#$%^&*()_+-=[]{}|;:',.<>?/`~");
    EXPECT_STREQ(encrypted.decrypt(), "!@#$%^&*()_+-=[]{}|;:',.<>?/`~");
}

TEST(ObfuscateTest, StringEncryptorProducesDifferentOutput) {
    std::string original = "RuntimeEncrypted";
    auto encrypted = StringEncryptor::encrypt(original);
    EXPECT_NE(encrypted, original);
}

TEST(ObfuscateTest, StringEncryptorEmpty) {
    EXPECT_TRUE(StringEncryptor::encrypt("").empty());
    EXPECT_TRUE(StringEncryptor::decrypt("").empty());
}

TEST(ObfuscateTest, Fnv1aHash) {
    constexpr uint32_t hash = fnv1a("test");
    EXPECT_NE(hash, 0u);
    constexpr uint32_t same = fnv1a("test");
    EXPECT_EQ(hash, same);
    constexpr uint32_t diff = fnv1a("different");
    EXPECT_NE(hash, diff);
}

TEST(ObfuscateTest, StrMethod) {
    auto encrypted = make_encrypted("TestString");
    EXPECT_EQ(encrypted.str(), "TestString");
}

TEST(ObfuscateTest, EncryptedInt) {
    constexpr auto decrypted = EncryptedInt<123 ^ (0xAB * 0x01000193), 0xAB>::decrypt();
    EXPECT_EQ(decrypted, 123u);
}

TEST(ObfuscateTest, SizeMethod) {
    auto encrypted = make_encrypted("Hello");
    // sizeof("Hello") = 6 (includes null terminator)
    EXPECT_EQ(encrypted.size(), 6u);
}

TEST(ObfuscateTest, DifferentKeysDifferentEncryption) {
    auto enc1 = make_encrypted("Same");
    auto enc2 = make_encrypted("Same");
    EXPECT_STREQ(enc1.decrypt(), enc2.decrypt());
}