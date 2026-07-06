//
//  MockHttpClientTests.cpp
//  PureMVC Core tests
//

#include <gtest/gtest.h>

#include "Infrastructure/Http/MockHttpClient.hpp"
#include "Mocks/SyncExecutor.hpp"

using namespace core;

TEST(MockHttpClient, ReturnsCannedLoginSuccess) {
    test::SyncExecutor executor;
    MockHttpClient client(executor);

    HttpResponse captured;
    client.send(HttpRequest{}, [&captured](const HttpResponse& r) { captured = r; });

    EXPECT_TRUE(captured.ok());
    EXPECT_EQ(captured.status, 200);
    EXPECT_NE(captured.body.find("mock-access-token"), std::string::npos);
    EXPECT_NE(captured.body.find("refresh_token"), std::string::npos);
}
