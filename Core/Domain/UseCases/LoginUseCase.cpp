//
//  LoginUseCase.cpp
//  PureMVC Core
//

#include "LoginUseCase.hpp"

namespace core {

LoginUseCase::LoginUseCase(IAuthRepository& repository, ITokenStore& tokenStore)
    : repository_(repository), tokenStore_(tokenStore) {}

void LoginUseCase::execute(const LoginCredentials& credentials, Callback callback) {
    // Input validation is a business rule — fail fast, do not hit the network.
    if (credentials.email.empty()) {
        callback(false, "Email is required");
        return;
    }
    if (credentials.password.empty()) {
        callback(false, "Password is required");
        return;
    }

    ITokenStore& store = tokenStore_;
    repository_.login(credentials,
        [callback, &store](bool success, const AuthSession& session, const DomainError& error) {
            if (success) {
                // Persist tokens on successful authentication.
                store.save(session.token);
                callback(true, "Login successful");
            } else {
                const std::string message =
                    error.message.empty() ? "Login failed" : error.message;
                callback(false, message);
            }
        });
}

} // namespace core
