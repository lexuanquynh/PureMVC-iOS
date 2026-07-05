//
//  FakeAuthRepository.hpp
//  PureMVC Core tests
//
//  In-memory IAuthRepository that responds synchronously and records the
//  credentials it was called with, so tests stay deterministic (no network,
//  no threads).
//

#ifndef PUREMVC_CORE_FAKE_AUTH_REPOSITORY_HPP
#define PUREMVC_CORE_FAKE_AUTH_REPOSITORY_HPP

#include "Domain/Ports/IAuthRepository.hpp"

namespace core { namespace test {

class FakeAuthRepository : public IAuthRepository {
public:
    // Configuration for the next login() call.
    bool shouldSucceed = true;
    AuthSession sessionToReturn;
    DomainError errorToReturn;

    // Recorded interaction.
    int loginCallCount = 0;
    LoginCredentials lastCredentials;

    void login(const LoginCredentials& credentials, LoginCallback callback) override {
        ++loginCallCount;
        lastCredentials = credentials;
        if (shouldSucceed) {
            callback(true, sessionToReturn, DomainError{});
        } else {
            callback(false, AuthSession{}, errorToReturn);
        }
    }
};

}} // namespace core::test

#endif // PUREMVC_CORE_FAKE_AUTH_REPOSITORY_HPP
