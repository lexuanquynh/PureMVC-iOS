//
//  UserProxy.cpp
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "UserProxy.h"
#include "httplib.h"
#include "json.hpp"
#include <thread>
#include "NetworkManager.h"

const std::string UserProxy::NAME = "UserProxy";

UserProxy::UserProxy() : Proxy(NAME) {
    userData.isLogin = false;
        
        // Initialize network manager with configuration
        RequestConfig config;
        config.baseUrl = "sample.com";
        config.port = 443;
        config.useSSL = true;
        config.verifySSL = false;
        config.connectionTimeout = 10;
        config.readTimeout = 10;
        
        // Set default headers
        config.defaultHeaders = {
            {"Content-Type", "application/json"},
        };
        
        // Configure retry for token expiration
        config.maxRetries = 3;
        config.retryDelayMs = 1000;
        config.retryOnStatusCodes = {401, 403, 503}; // Will retry on unauthorized
        
        networkManager_ = new NetworkManager(config);
        
        // Create and set token provider
        tokenProvider_ = new UserTokenProvider(networkManager_);
        networkManager_->setTokenProvider(tokenProvider_);
        networkManager_->setAutoRefreshToken(true); // Enable auto refresh
        
        // Optional: Set request interceptor for custom logic
        networkManager_->setRequestInterceptor([](httplib::Headers& headers) {
            // Add any custom headers or modify existing ones before each request
            // For example, add timestamp or request ID
            headers.emplace("X-Request-Time", std::to_string(std::time(nullptr)));
        });
}

UserProxy::~UserProxy() {
    if (tokenProvider_) {
        delete tokenProvider_;
    }
    if (networkManager_) {
        delete networkManager_;
    }
}

void UserProxy::login(const std::string& username,
                      const std::string& password,
                      std::function<void(bool success, const std::string& message)> callback) {
    
    // Create JSON request body
    json requestBody = {
        {"email", username},
        {"password", password}
    };
    
    // Make async POST request
    networkManager_->post("/api/v1/auth/login", requestBody, [this, username, callback](const NetworkResponse& response) {
        // Log response for debugging
//        std::cout << "Login response status: " << response.statusCode << std::endl;
//        std::cout << "Login response body: " << response.body << std::endl;
        
        if (response.success) {
            try {
                json jsonResponse = json::parse(response.body);
                
                // Extract data from response
                userData.username = username;
                userData.accessToken = jsonResponse.value("access_token", "");
                userData.refreshToken = jsonResponse.value("refresh_token", "");
                userData.isVerify = jsonResponse.value("is_verify", false);
                userData.isLogin = true;
                
                // Update token provider with new tokens
                tokenProvider_->setTokens(userData.accessToken, userData.refreshToken);
                
                // Also update network manager directly
                networkManager_->setAccessToken(userData.accessToken);
                networkManager_->setRefreshToken(userData.refreshToken);
                
                callback(true, "Login successful");
            } catch (const std::exception& e) {
                userData.isLogin = false;
                callback(false, std::string("Failed to parse response: ") + e.what());
            }
        } else {
            userData.isLogin = false;
            callback(false, response.errorMessage);
        }
    });
}

void UserProxy::logout() {
    userData.accessToken = "";
    userData.refreshToken = "";
    userData.isVerify = false;
    userData.isLogin = false;
}

bool UserProxy::isLoggedIn() const {
    return userData.isLogin;
}

std::string UserProxy::getUsername() const {
    return userData.username;
}
