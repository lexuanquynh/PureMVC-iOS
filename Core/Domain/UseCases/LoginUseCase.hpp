//
//  LoginUseCase.hpp
//  PureMVC Core — application use case
//
//  Business rule for logging a user in. Depends only on domain ports,
//  so it is fully unit-testable without network or platform code.
//

#ifndef PUREMVC_CORE_LOGIN_USE_CASE_HPP
#define PUREMVC_CORE_LOGIN_USE_CASE_HPP

#include <functional>
#include "../Ports/IAuthRepository.hpp"
#include "../Ports/ITokenStore.hpp"

namespace core {

class LoginUseCase {
public:
    // Reports the outcome to the caller (e.g. a PureMVC Command) in
    // platform-neutral terms.
    using Callback = std::function<void(bool success, const std::string& message)>;

    LoginUseCase(IAuthRepository& repository, ITokenStore& tokenStore);

    // Validates input, delegates to the repository, and on success persists
    // the returned tokens before reporting back.
    void execute(const LoginCredentials& credentials, Callback callback);

private:
    IAuthRepository& repository_;
    ITokenStore& tokenStore_;
};

} // namespace core

#endif // PUREMVC_CORE_LOGIN_USE_CASE_HPP
