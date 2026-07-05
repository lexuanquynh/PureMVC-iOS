//
//  IAuthRepository.hpp
//  PureMVC Core — outbound port
//
//  The domain talks to the outside world (network) through this interface.
//  Infrastructure provides a concrete impl over httplib; tests provide a fake.
//

#ifndef PUREMVC_CORE_IAUTH_REPOSITORY_HPP
#define PUREMVC_CORE_IAUTH_REPOSITORY_HPP

#include <functional>
#include "../AuthTypes.hpp"

namespace core {

class IAuthRepository {
public:
    // Called with success == true and a valid session, or success == false
    // and a populated error. Callback may fire on any thread; the use case
    // does not assume a specific one.
    using LoginCallback =
        std::function<void(bool success, const AuthSession& session, const DomainError& error)>;

    virtual void login(const LoginCredentials& credentials, LoginCallback callback) = 0;

    virtual ~IAuthRepository() = default;
};

} // namespace core

#endif // PUREMVC_CORE_IAUTH_REPOSITORY_HPP
