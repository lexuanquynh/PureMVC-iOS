//
//  AuthRepositoryTests.cpp
//  PureMVC Core tests
//
//  Exercises the infrastructure repository with a fake HTTP client — no real
//  network, no simulator.
//

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Infrastructure/Auth/AuthRepository.hpp"
#include "Mocks/FakeHttpClient.hpp"

using namespace core;
using json = nlohmann::json;

namespace {

struct RepoOutcome {
    bool called = false;
    bool success = false;
    AuthSession session;
    DomainError error;
};

IAuthRepository::LoginCallback capture(RepoOutcome& out) {
    return [&out](bool success, const AuthSession& session, const DomainError& error) {
        out.called = true;
        out.success = success;
        out.session = session;
        out.error = error;
    };
}

} // namespace

TEST(AuthRepository, BuildsPostJsonRequestToLoginPath) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 200;
    http.responseToReturn.body = "{}";
    AuthRepository repo(http, "/api/v1/auth/login");

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "password123"}, capture(out));

    ASSERT_EQ(http.sendCallCount, 1);
    EXPECT_EQ(http.lastRequest.method, "POST");
    EXPECT_EQ(http.lastRequest.path, "/api/v1/auth/login");
    EXPECT_EQ(http.lastRequest.contentType, "application/json");

    json sent = json::parse(http.lastRequest.body);
    EXPECT_EQ(sent.at("email"), "user@example.com");
    EXPECT_EQ(sent.at("password"), "password123");
}

TEST(AuthRepository, UsesConfiguredLoginPath) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 200;
    http.responseToReturn.body = "{}";
    AuthRepository repo(http, "/v2/signin");

    RepoOutcome out;
    repo.login(LoginCredentials{"a@b.com", "pw"}, capture(out));

    EXPECT_EQ(http.lastRequest.path, "/v2/signin");
}

TEST(AuthRepository, SuccessParsesTokensAndSession) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 200;
    http.responseToReturn.body =
        R"({"access_token":"acc-1","refresh_token":"ref-1","is_verify":true})";
    AuthRepository repo(http);

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "password123"}, capture(out));

    ASSERT_TRUE(out.called);
    EXPECT_TRUE(out.success);
    EXPECT_EQ(out.session.user.username, "user@example.com"); // carried from request
    EXPECT_TRUE(out.session.user.isVerified);
    EXPECT_EQ(out.session.token.accessToken, "acc-1");
    EXPECT_EQ(out.session.token.refreshToken, "ref-1");
}

TEST(AuthRepository, HttpErrorMapsStatusToDefaultMessage) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 401;
    http.responseToReturn.body = "";
    AuthRepository repo(http);

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "wrong"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.error.code, 401);
    EXPECT_EQ(out.error.message, "Invalid credentials");
}

TEST(AuthRepository, HttpErrorPrefersMessageFromBody) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 400;
    http.responseToReturn.body = R"({"message":"Email already registered"})";
    AuthRepository repo(http);

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "pw"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.error.code, 400);
    EXPECT_EQ(out.error.message, "Email already registered");
}

TEST(AuthRepository, MalformedJsonOnSuccessReportsParseError) {
    test::FakeHttpClient http;
    http.responseToReturn.status = 200;
    http.responseToReturn.body = "this is not json";
    AuthRepository repo(http);

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "pw"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.error.message.rfind("Failed to parse response:", 0), 0u);
}

TEST(AuthRepository, TransportErrorIsReportedWithCodeZero) {
    test::FakeHttpClient http;
    http.responseToReturn.transportError = true;
    http.responseToReturn.transportErrorMessage = "Timed out";
    AuthRepository repo(http);

    RepoOutcome out;
    repo.login(LoginCredentials{"user@example.com", "pw"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.error.code, 0);
    EXPECT_EQ(out.error.message, "Timed out");
}
