//
//  NetworkManager.h
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#ifndef NetworkManager_h
#define NetworkManager_h

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include <string>
#include <functional>
#include <map>
#include <memory>
#include <mutex>

using json = nlohmann::json;

// Response structure to hold HTTP response data
struct NetworkResponse {
    int statusCode;
    std::string body;
    httplib::Headers headers;
    bool success;
    std::string errorMessage;
    
    NetworkResponse() : statusCode(0), success(false) {}
};

// Request configuration
struct RequestConfig {
    std::string baseUrl;
    int port;
    bool useSSL;
    int connectionTimeout;
    int readTimeout;
    bool verifySSL;
    httplib::Headers defaultHeaders;
    
    // Retry configuration
    int maxRetries;
    int retryDelayMs;
    std::vector<int> retryOnStatusCodes; // Status codes that trigger retry
    
    RequestConfig() :
        port(443),
        useSSL(true),
        connectionTimeout(30),
        readTimeout(30),
        verifySSL(true),
        maxRetries(3),
        retryDelayMs(1000),
        retryOnStatusCodes({401, 403, 503}) {}
};

// Token provider interface
class TokenProvider {
public:
    virtual ~TokenProvider() = default;
    virtual std::string getAccessToken() = 0;
    virtual std::string getRefreshToken() = 0;
    virtual void refreshAccessToken(std::function<void(bool success, const std::string& newToken)> callback) = 0;
};

// Request context for internal use
struct RequestContext {
    std::string path;
    std::string method;
    httplib::Headers headers;
    httplib::Params params;
    std::string body;
    std::string contentType;
    httplib::MultipartFormDataItems multipartItems; // For multipart form data
    int retryCount;
    std::function<void(const NetworkResponse&)> callback;
    
    RequestContext() : retryCount(0) {}
};

class NetworkManager {
public:
    using ResponseCallback = std::function<void(const NetworkResponse&)>;
    using InterceptorCallback = std::function<void(httplib::Headers& headers)>;
    
    // Constructor with configuration
    explicit NetworkManager(const RequestConfig& config = RequestConfig());
    ~NetworkManager();
    // Internal request execution
    httplib::Result executeClientRequest(const std::string& method, const std::string& path,
                                         const httplib::Headers& headers, const std::string& body,
                                         const std::string& contentType);
    
    // Token management
    void setTokenProvider(TokenProvider* provider);
    void setAccessToken(const std::string& token);
    void setRefreshToken(const std::string& token);
    void clearTokens();
    
    // Request interceptor (called before each request)
    void setRequestInterceptor(InterceptorCallback interceptor);
    
    // GET request methods
    void get(const std::string& path, ResponseCallback callback);
    void get(const std::string& path, const httplib::Headers& headers, ResponseCallback callback);
    void get(const std::string& path, const httplib::Params& params, ResponseCallback callback);
    void get(const std::string& path, const httplib::Headers& headers, const httplib::Params& params, ResponseCallback callback);
    
    // POST request methods - JSON
    void post(const std::string& path, const json& jsonData, ResponseCallback callback);
    void post(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback);
    
    // POST request methods - Form data
    void postForm(const std::string& path, const httplib::Params& params, ResponseCallback callback);
    void postForm(const std::string& path, const httplib::Params& params, const httplib::Headers& headers, ResponseCallback callback);
    
    // POST request methods - Multipart form data
    void postMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, ResponseCallback callback);
    void postMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, const httplib::Headers& headers, ResponseCallback callback);
    
    // POST request methods - Raw body
    void postRaw(const std::string& path, const std::string& body, const std::string& contentType, ResponseCallback callback);
    void postRaw(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers, ResponseCallback callback);
    
    // PUT request methods
    void put(const std::string& path, const json& jsonData, ResponseCallback callback);
    void put(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback);
    void putRaw(const std::string& path, const std::string& body, const std::string& contentType, ResponseCallback callback);
    void putRaw(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers, ResponseCallback callback);
    
    // DELETE request methods
    void del(const std::string& path, ResponseCallback callback);
    void del(const std::string& path, const httplib::Headers& headers, ResponseCallback callback);
    
    // PATCH request methods
    void patch(const std::string& path, const json& jsonData, ResponseCallback callback);
    void patch(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback);
    
    // Configuration methods
    void setDefaultHeader(const std::string& key, const std::string& value);
    void removeDefaultHeader(const std::string& key);
    void setTimeout(int connectionTimeout, int readTimeout);
    void setSSLVerification(bool verify);
    void setRetryConfig(int maxRetries, int retryDelayMs, const std::vector<int>& statusCodes);
    
    // Enable/disable automatic token refresh
    void setAutoRefreshToken(bool enable);
    
private:
    RequestConfig config_;
    httplib::SSLClient* sslClient_;
    httplib::Client* httpClient_;
    
    // Token management
    std::string accessToken_;
    std::string refreshToken_;
    TokenProvider* tokenProvider_;
    bool autoRefreshToken_;
    std::mutex tokenMutex_;
   
    // Request interceptor
    InterceptorCallback requestInterceptor_;
    
    // Helper methods
    void initializeClient();
    httplib::Headers mergeHeaders(const httplib::Headers& requestHeaders) const;
    NetworkResponse processResponse(const httplib::Result& res) const;
    void executeAsync(std::function<void()> task);
    httplib::Client* getClient();
    
    // Token injection
    void injectAuthToken(httplib::Headers& headers);
    
    // Retry logic
    bool shouldRetry(int statusCode, int retryCount) const;
    void executeWithRetry(RequestContext& context);
    void handleTokenRefresh(RequestContext& context);
    
    // Internal request execution
    NetworkResponse executeRequest(const RequestContext& context);
    
    // Internal request methods
    NetworkResponse doGet(const std::string& path, const httplib::Headers& headers, const httplib::Params& params);
    NetworkResponse doPost(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers);
    NetworkResponse doPut(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers);
    NetworkResponse doDelete(const std::string& path, const httplib::Headers& headers);
    NetworkResponse doPatch(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers);
    NetworkResponse doPostMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, const httplib::Headers& headers);
};

// Simple token provider implementation
class SimpleTokenProvider : public TokenProvider {
public:
    SimpleTokenProvider() {}
    
    void setTokens(const std::string& access, const std::string& refresh) {
        std::lock_guard<std::mutex> lock(mutex_);
        accessToken_ = access;
        refreshToken_ = refresh;
    }
    
    std::string getAccessToken() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return accessToken_;
    }
    
    std::string getRefreshToken() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return refreshToken_;
    }
    
    void refreshAccessToken(std::function<void(bool success, const std::string& newToken)> callback) override {
        // This should be overridden to implement actual refresh logic
        callback(false, "");
    }
    
private:
    std::string accessToken_;
    std::string refreshToken_;
    std::mutex mutex_;
};

// Singleton pattern for global access (optional)
class NetworkService {
public:
    static NetworkService& getInstance() {
        static NetworkService instance;
        return instance;
    }
    
    void initialize(const RequestConfig& config) {
        if (manager_) {
            delete manager_;
        }
        manager_ = new NetworkManager(config);
    }
    
    NetworkManager* getManager() {
        return manager_;
    }
    
private:
    NetworkService() : manager_(nullptr) {}
    ~NetworkService() {
        if (manager_) {
            delete manager_;
        }
    }
    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;
    
    NetworkManager* manager_;
};

#endif /* NetworkManager_h */
