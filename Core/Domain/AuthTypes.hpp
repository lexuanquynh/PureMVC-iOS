//
//  AuthTypes.hpp
//  PureMVC Core — shared value objects for the auth domain
//
//  Kept C++11-compatible (no std::optional/variant) so the same headers
//  can compile into the iOS target, which builds at gnu++11.
//

#ifndef PUREMVC_CORE_AUTH_TYPES_HPP
#define PUREMVC_CORE_AUTH_TYPES_HPP

#include <string>
#include "Entities/User.hpp"
#include "Entities/Token.hpp"

namespace core {

// Input for a login attempt.
struct LoginCredentials {
    std::string email;
    std::string password;
};

// Authenticated result: who logged in + their tokens.
struct AuthSession {
    User user;
    Token token;
};

// A domain-level error, decoupled from HTTP/platform error types.
struct DomainError {
    std::string message;
    int code = 0; // 0 = unspecified; infrastructure may map HTTP status here

    DomainError() = default;
    explicit DomainError(std::string msg, int c = 0)
        : message(std::move(msg)), code(c) {}
};

} // namespace core

#endif // PUREMVC_CORE_AUTH_TYPES_HPP
