//
//  AuthRepository.hpp
//  PureMVC Core — Infrastructure
//
//  Concrete IAuthRepository that turns domain credentials into an HTTP call via
//  IHttpClient and maps the response back into domain types. JSON handling is an
//  implementation detail of the .cpp — it is not exposed here.
//

#ifndef PUREMVC_CORE_AUTH_REPOSITORY_HPP
#define PUREMVC_CORE_AUTH_REPOSITORY_HPP

#include <string>
#include "Domain/Ports/IAuthRepository.hpp"
#include "Infrastructure/Http/IHttpClient.hpp"

namespace core {

class AuthRepository : public IAuthRepository {
public:
    explicit AuthRepository(IHttpClient& httpClient,
                            std::string loginPath = "/api/v1/auth/login");

    void login(const LoginCredentials& credentials, LoginCallback callback) override;

private:
    IHttpClient& httpClient_;
    std::string loginPath_;
};

} // namespace core

#endif // PUREMVC_CORE_AUTH_REPOSITORY_HPP
