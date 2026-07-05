//
//  LoginUseCaseTests.cpp
//  PureMVC Core tests
//
//  Proves the domain layer is testable on the host with no simulator,
//  no network, and no Objective-C.
//

#include <gtest/gtest.h>

#include "Domain/UseCases/LoginUseCase.hpp"
#include "Mocks/FakeAuthRepository.hpp"
#include "Mocks/FakeTokenStore.hpp"

using namespace core;

namespace {

struct LoginOutcome {
    bool called = false;
    bool success = false;
    std::string message;
};

LoginUseCase::Callback capture(LoginOutcome& out) {
    return [&out](bool success, const std::string& message) {
        out.called = true;
        out.success = success;
        out.message = message;
    };
}

} // namespace

TEST(LoginUseCase, EmptyEmailFailsFastWithoutHittingRepository) {
    test::FakeAuthRepository repo;
    test::FakeTokenStore store;
    LoginUseCase useCase(repo, store);

    LoginOutcome out;
    useCase.execute(LoginCredentials{"", "password123"}, capture(out));

    EXPECT_TRUE(out.called);
    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.message, "Email is required");
    EXPECT_EQ(repo.loginCallCount, 0);   // business rule: never reach network
    EXPECT_EQ(store.saveCallCount, 0);
}

TEST(LoginUseCase, EmptyPasswordFailsFast) {
    test::FakeAuthRepository repo;
    test::FakeTokenStore store;
    LoginUseCase useCase(repo, store);

    LoginOutcome out;
    useCase.execute(LoginCredentials{"user@example.com", ""}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.message, "Password is required");
    EXPECT_EQ(repo.loginCallCount, 0);
}

TEST(LoginUseCase, SuccessfulLoginPersistsTokensAndReports) {
    test::FakeAuthRepository repo;
    repo.shouldSucceed = true;
    repo.sessionToReturn.user.username = "user@example.com";
    repo.sessionToReturn.token.accessToken = "access-abc";
    repo.sessionToReturn.token.refreshToken = "refresh-xyz";

    test::FakeTokenStore store;
    LoginUseCase useCase(repo, store);

    LoginOutcome out;
    useCase.execute(LoginCredentials{"user@example.com", "password123"}, capture(out));

    EXPECT_TRUE(out.success);
    EXPECT_EQ(out.message, "Login successful");
    EXPECT_EQ(repo.loginCallCount, 1);
    EXPECT_EQ(repo.lastCredentials.email, "user@example.com");

    // Tokens must be persisted through the store port.
    EXPECT_EQ(store.saveCallCount, 1);
    EXPECT_EQ(store.stored.accessToken, "access-abc");
    EXPECT_EQ(store.stored.refreshToken, "refresh-xyz");
}

TEST(LoginUseCase, FailedLoginPropagatesMessageAndDoesNotPersist) {
    test::FakeAuthRepository repo;
    repo.shouldSucceed = false;
    repo.errorToReturn = DomainError{"Invalid credentials", 401};

    test::FakeTokenStore store;
    LoginUseCase useCase(repo, store);

    LoginOutcome out;
    useCase.execute(LoginCredentials{"user@example.com", "wrong"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.message, "Invalid credentials");
    EXPECT_EQ(repo.loginCallCount, 1);
    EXPECT_EQ(store.saveCallCount, 0);   // never persist on failure
}

TEST(LoginUseCase, FailedLoginWithoutMessageFallsBackToDefault) {
    test::FakeAuthRepository repo;
    repo.shouldSucceed = false;
    repo.errorToReturn = DomainError{}; // empty message

    test::FakeTokenStore store;
    LoginUseCase useCase(repo, store);

    LoginOutcome out;
    useCase.execute(LoginCredentials{"user@example.com", "wrong"}, capture(out));

    EXPECT_FALSE(out.success);
    EXPECT_EQ(out.message, "Login failed");
}
