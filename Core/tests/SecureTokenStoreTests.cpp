//
//  SecureTokenStoreTests.cpp
//  PureMVC Core tests
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Infrastructure/Security/SecureTokenStore.hpp"
#include "Mocks/FakeSecureStore.hpp"

using namespace core;
using json = nlohmann::json;

TEST(SecureTokenStore, SaveThenLoadRoundTrips) {
    test::FakeSecureStore storage;
    SecureTokenStore store(storage);

    Token token;
    token.accessToken = "acc-1";
    token.refreshToken = "ref-1";
    store.save(token);

    Token loaded = store.load();
    EXPECT_EQ(loaded.accessToken, "acc-1");
    EXPECT_EQ(loaded.refreshToken, "ref-1");
}

TEST(SecureTokenStore, LoadWithNothingStoredReturnsEmptyToken) {
    test::FakeSecureStore storage;
    SecureTokenStore store(storage);

    Token loaded = store.load();
    EXPECT_TRUE(loaded.empty());
    EXPECT_TRUE(loaded.refreshToken.empty());
}

TEST(SecureTokenStore, ClearRemovesStoredEntry) {
    test::FakeSecureStore storage;
    SecureTokenStore store(storage);

    Token token;
    token.accessToken = "acc-1";
    token.refreshToken = "ref-1";
    store.save(token);

    store.clear();

    EXPECT_EQ(storage.removeCount, 1);
    EXPECT_TRUE(store.load().empty());
}

TEST(SecureTokenStore, MalformedBlobReturnsEmptyToken) {
    test::FakeSecureStore storage;
    SecureTokenStore store(storage, "com.puremvc.auth.tokens");
    storage.data["com.puremvc.auth.tokens"] = "not-json";

    Token loaded = store.load();
    EXPECT_TRUE(loaded.empty());
    EXPECT_TRUE(loaded.refreshToken.empty());
}

TEST(SecureTokenStore, UsesConfiguredKeyAndSerializesBothFields) {
    test::FakeSecureStore storage;
    SecureTokenStore store(storage, "custom.key");

    Token token;
    token.accessToken = "a";
    token.refreshToken = "r";
    store.save(token);

    ASSERT_EQ(storage.setCount, 1);
    ASSERT_NE(storage.data.find("custom.key"), storage.data.end());

    json stored = json::parse(storage.data["custom.key"]);
    EXPECT_EQ(stored.at("access_token"), "a");
    EXPECT_EQ(stored.at("refresh_token"), "r");
}
