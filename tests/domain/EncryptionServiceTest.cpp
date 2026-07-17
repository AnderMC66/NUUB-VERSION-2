#include <gtest/gtest.h>

#include "domain/services/EncryptionService.hpp"

using namespace nuub::domain::services;

TEST(EncryptionServiceTest, EncryptDecryptRoundtrip) {
    EncryptionService enc("test-password-123");

    std::vector<unsigned char> plaintext = {'H', 'e', 'l', 'l', 'o'};
    auto encrypted = enc.encrypt(plaintext);

    // Encrypted should be different from plaintext
    EXPECT_NE(encrypted, plaintext);
    EXPECT_GT(encrypted.size(), plaintext.size());

    // Decrypt should recover original
    auto decrypted = enc.decrypt(encrypted);
    EXPECT_EQ(decrypted, plaintext);
}

TEST(EncryptionServiceTest, DifferentSaltsProducesDifferentCiphertext) {
    EncryptionService enc("same-password");

    std::vector<unsigned char> data = {'T', 'e', 's', 't'};
    auto enc1 = enc.encrypt(data);
    auto enc2 = enc.encrypt(data);

    // With random salts, ciphertext should differ
    EXPECT_NE(enc1, enc2);
}

TEST(EncryptionServiceTest, WrongPasswordFails) {
    EncryptionService enc1("correct-password");
    EncryptionService enc2("wrong-password");

    auto encrypted = enc1.encrypt({'D', 'a', 't', 'a'});
    EXPECT_THROW(enc2.decrypt(encrypted), std::runtime_error);
}

TEST(EncryptionServiceTest, EmptyData) {
    EncryptionService enc("password");
    std::vector<unsigned char> empty;
    auto encrypted = enc.encrypt(empty);
    auto decrypted = enc.decrypt(encrypted);
    EXPECT_EQ(decrypted, empty);
}

// New tests for GCM, AAD, and key rotation

TEST(EncryptionServiceTest, AADEncryptionRoundtrip) {
    EncryptionService enc("test-password");

    std::vector<unsigned char> plaintext = {'S', 'e', 'c', 'r', 'e', 't'};
    std::vector<unsigned char> aad = {'P', 'C', '-', '0', '0', '1'};

    auto encrypted = enc.encrypt(plaintext, aad);
    auto decrypted = enc.decrypt(encrypted, aad);

    EXPECT_EQ(decrypted, plaintext);
}

TEST(EncryptionServiceTest, WrongAADFails) {
    EncryptionService enc("test-password");

    std::vector<unsigned char> plaintext = {'D', 'a', 't', 'a'};
    std::vector<unsigned char> aad1 = {'P', 'C', '-', '0', '0', '1'};
    std::vector<unsigned char> aad2 = {'P', 'C', '-', '0', '0', '2'};

    auto encrypted = enc.encrypt(plaintext, aad1);
    EXPECT_THROW(enc.decrypt(encrypted, aad2), std::runtime_error);
}

TEST(EncryptionServiceTest, KeyRotationChangesKeyId) {
    EncryptionService enc("password");

    EXPECT_EQ(enc.get_current_key_id(), 1u);

    auto enc1 = enc.encrypt({'D', '1'});
    enc.rotate_key();

    EXPECT_EQ(enc.get_current_key_id(), 2u);

    auto enc2 = enc.encrypt({'D', '2'});

    // Old key_id should fail to decrypt with new key
    EXPECT_THROW(enc.decrypt(enc1), std::runtime_error);

    // New data should decrypt fine
    auto decrypted = enc.decrypt(enc2);
    std::vector<unsigned char> expected = {'D', '2'};
    EXPECT_EQ(decrypted, expected);
}

TEST(EncryptionServiceTest, EncryptedDataHasCorrectHeaderFormat) {
    EncryptionService enc("password");

    std::vector<unsigned char> data = {'T', 'e', 's', 't'};
    auto encrypted = enc.encrypt(data);

    // Header: version(1) + key_id(4) + salt(32) + nonce(12) = 49 bytes
    // Plus tag(16) = 65 bytes minimum
    EXPECT_GE(encrypted.size(), 65u);

    // Version byte should be 0x02
    EXPECT_EQ(encrypted[0], 0x02);

    // Key ID should be 0x00000001 (big-endian)
    EXPECT_EQ(encrypted[1], 0x00);
    EXPECT_EQ(encrypted[2], 0x00);
    EXPECT_EQ(encrypted[3], 0x00);
    EXPECT_EQ(encrypted[4], 0x01);
}

TEST(EncryptionServiceTest, TamperedCiphertextFails) {
    EncryptionService enc("password");

    std::vector<unsigned char> data = {'S', 'e', 'c', 'r', 'e', 't'};
    auto encrypted = enc.encrypt(data);

    // Tamper with ciphertext (modify a byte in the middle)
    size_t mid = encrypted.size() / 2;
    encrypted[mid] ^= 0xFF;

    EXPECT_THROW(enc.decrypt(encrypted), std::runtime_error);
}

TEST(EncryptionServiceTest, TamperedTagFails) {
    EncryptionService enc("password");

    std::vector<unsigned char> data = {'S', 'e', 'c', 'r', 'e', 't'};
    auto encrypted = enc.encrypt(data);

    // Tamper with authentication tag (last byte)
    encrypted.back() ^= 0xFF;

    EXPECT_THROW(enc.decrypt(encrypted), std::runtime_error);
}
