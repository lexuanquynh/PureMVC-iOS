//
//  user_token_provider.h
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#ifndef user_token_provider_h
#define user_token_provider_h

// Custom token provider for user authentication
class UserTokenProvider : public TokenProvider {
public:
    UserTokenProvider(NetworkManager* networkManager) : networkManager_(networkManager) {}
    
    void setTokens(const std::string& access, const std::string& refresh) {
        accessToken_ = access;
        refreshToken_ = refresh;
    }
    
    std::string getAccessToken() override {
        return accessToken_;
    }
    
    std::string getRefreshToken() override {
        return refreshToken_;
    }
    
    void refreshAccessToken(std::function<void(bool success, const std::string& newToken)> callback) override {
        if (refreshToken_.empty()) {
            callback(false, "");
            return;
        }
        
        // Create a temporary network manager without token injection to avoid recursion
        RequestConfig config;
        config.baseUrl = "chat.codetoanbug.com";
        config.port = 443;
        config.useSSL = true;
        config.verifySSL = false;
        
        NetworkManager tempNetwork(config);
        
        json requestBody = {
            {"refresh_token", refreshToken_}
        };
        
        tempNetwork.post("/api/v1/auth/refresh", requestBody, [this, callback](const NetworkResponse& response) {
            if (response.success) {
                try {
                    json jsonResponse = json::parse(response.body);
                    std::string newAccessToken = jsonResponse.value("access_token", "");
                    std::string newRefreshToken = jsonResponse.value("refresh_token", "");
                    
                    if (!newAccessToken.empty()) {
                        accessToken_ = newAccessToken;
                        if (!newRefreshToken.empty()) {
                            refreshToken_ = newRefreshToken;
                        }
                        callback(true, newAccessToken);
                    } else {
                        callback(false, "");
                    }
                } catch (...) {
                    callback(false, "");
                }
            } else {
                callback(false, "");
            }
        });
    }
    
private:
    std::string accessToken_;
    std::string refreshToken_;
    NetworkManager* networkManager_;
};
#endif /* user_token_provider_h */
