//
//  UserProxy.hpp
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#ifndef USER_PROXY_H
#define USER_PROXY_H

#include "PureMVC/Patterns/Proxy/Proxy.hpp"
#include <string>
#include "NetworkManager.h"
#include "user_token_provider.h"

using namespace PureMVC;
using namespace PureMVC::Patterns;

class UserProxy : public Proxy {
private:
    struct UserData {
        std::string username;
        std::string accessToken;
        std::string refreshToken;
        bool isVerify;
        bool isLogin;
    };
    
    UserData userData;
    NetworkManager* networkManager_;
    UserTokenProvider* tokenProvider_;
public:
    static const std::string NAME;
    
    UserProxy();
    ~UserProxy();
    
//    bool login(const std::string& username, const std::string& password);
    void login(const std::string& username,
                   const std::string& password,
                   std::function<void(bool success, const std::string& message)> callback);
    void logout();
    bool isLoggedIn() const;
    std::string getUsername() const;
};

#endif // USER_PROXY_H
