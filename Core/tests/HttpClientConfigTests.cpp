//
//  HttpClientConfigTests.cpp
//  PureMVC Core tests
//
//  Guards the secure-by-default posture of the HTTP client configuration.
//

#include <gtest/gtest.h>

#include "Infrastructure/Http/HttpClientConfig.hpp"

using namespace core;

TEST(HttpClientConfig, IsSecureByDefault) {
    HttpClientConfig config;
    EXPECT_TRUE(config.useSSL);
    EXPECT_TRUE(config.verifySSL);                       // verification ON by default
    EXPECT_TRUE(config.caCertPath.empty());              // system trust store
    EXPECT_TRUE(config.pinnedSpkiSha256Base64.empty());  // pinning opt-in
}
