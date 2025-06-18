//
//  NetworkManager.cpp
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#include "NetworkManager.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include "string_helper.h"
#include "http_errors.h"

NetworkManager::NetworkManager(const RequestConfig& config) :
    config_(config),
    sslClient_(nullptr),
    httpClient_(nullptr),
    tokenProvider_(nullptr),
    autoRefreshToken_(true) {
    initializeClient();
}

NetworkManager::~NetworkManager() {
    if (sslClient_) {
        delete sslClient_;
    }
    if (httpClient_) {
        delete httpClient_;
    }
}

void NetworkManager::initializeClient() {
    if (config_.useSSL) {
        sslClient_ = new httplib::SSLClient(config_.baseUrl, config_.port);
        sslClient_->enable_server_certificate_verification(config_.verifySSL);
        sslClient_->set_connection_timeout(config_.connectionTimeout, 0);
        sslClient_->set_read_timeout(config_.readTimeout, 0);
    } else {
        httpClient_ = new httplib::Client(config_.baseUrl, config_.port);
        httpClient_->set_connection_timeout(config_.connectionTimeout, 0);
        httpClient_->set_read_timeout(config_.readTimeout, 0);
    }
}

httplib::Result NetworkManager::executeClientRequest(const std::string& method, const std::string& path,
                                                    const httplib::Headers& headers, const std::string& body,
                                                    const std::string& contentType) {
    if (config_.useSSL && sslClient_) {
        if (method == "GET") {
            return sslClient_->Get(path.c_str(), headers);
        } else if (method == "POST") {
            if (body.empty()) {
                return sslClient_->Post(path.c_str(), headers);
            } else {
                return sslClient_->Post(path.c_str(), headers, body, contentType.c_str());
            }
        } else if (method == "PUT") {
            return sslClient_->Put(path.c_str(), headers, body, contentType.c_str());
        } else if (method == "DELETE") {
            return sslClient_->Delete(path.c_str(), headers);
        } else if (method == "PATCH") {
            return sslClient_->Patch(path.c_str(), headers, body, contentType.c_str());
        }
    } else if (!config_.useSSL && httpClient_) {
        if (method == "GET") {
            return httpClient_->Get(path.c_str(), headers);
        } else if (method == "POST") {
            if (body.empty()) {
                return httpClient_->Post(path.c_str(), headers);
            } else {
                return httpClient_->Post(path.c_str(), headers, body, contentType.c_str());
            }
        } else if (method == "PUT") {
            return httpClient_->Put(path.c_str(), headers, body, contentType.c_str());
        } else if (method == "DELETE") {
            return httpClient_->Delete(path.c_str(), headers);
        } else if (method == "PATCH") {
            return httpClient_->Patch(path.c_str(), headers, body, contentType.c_str());
        }
    }

    // Return empty Result for error case
    return httplib::Result{nullptr, httplib::Error::Unknown};
}

// Token management
void NetworkManager::setTokenProvider(TokenProvider* provider) {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    tokenProvider_ = provider;
}

void NetworkManager::setAccessToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    accessToken_ = token;
}

void NetworkManager::setRefreshToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    refreshToken_ = token;
}

void NetworkManager::clearTokens() {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    accessToken_.clear();
    refreshToken_.clear();
}

void NetworkManager::setRequestInterceptor(InterceptorCallback interceptor) {
    requestInterceptor_ = interceptor;
}

void NetworkManager::setAutoRefreshToken(bool enable) {
    autoRefreshToken_ = enable;
}

void NetworkManager::injectAuthToken(httplib::Headers& headers) {
    std::lock_guard<std::mutex> lock(tokenMutex_);

    // Get token from provider if available
    std::string token = accessToken_;
    if (tokenProvider_) {
        token = tokenProvider_->getAccessToken();
    }

    if (!token.empty()) {
        // Remove existing Authorization header if present
        auto it = headers.begin();
        while (it != headers.end()) {
            if (it->first == "Authorization") {
                it = headers.erase(it);
            } else {
                ++it;
            }
        }
        // Add new Authorization header
        headers.emplace("Authorization", "Bearer " + token);
    }
}

httplib::Headers NetworkManager::mergeHeaders(const httplib::Headers& requestHeaders) const {
    httplib::Headers mergedHeaders = config_.defaultHeaders;

    // Merge request headers with default headers
    for (const auto& header : requestHeaders) {
        mergedHeaders.insert(header);
    }

    return mergedHeaders;
}

bool NetworkManager::shouldRetry(int statusCode, int retryCount) const {
    if (retryCount >= config_.maxRetries) {
        return false;
    }

    return std::find(config_.retryOnStatusCodes.begin(),
                     config_.retryOnStatusCodes.end(),
                     statusCode) != config_.retryOnStatusCodes.end();
}

void NetworkManager::executeWithRetry(RequestContext& context) {
    executeAsync([this, context]() mutable {
        NetworkResponse response = executeRequest(context);
        
        // Check if we should retry
        if (!response.success && shouldRetry(response.statusCode, context.retryCount)) {
            // If it's a 401/403 and we have auto refresh enabled, try to refresh token
            if (autoRefreshToken_ && tokenProvider_ &&
                (response.statusCode == 401 || response.statusCode == 403)) {
                // log
                std::cout << "Token refresh needed for status code: " << response.statusCode << std::endl;
                handleTokenRefresh(context);
                return;
            }
            
            // Otherwise, just retry after delay
            context.retryCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.retryDelayMs));
            executeWithRetry(context);
        } else {
            // Final response
            context.callback(response);
        }
    });
}

void NetworkManager::handleTokenRefresh(RequestContext& context) {
    if (!tokenProvider_) {
        NetworkResponse response;
        response.statusCode = 403;
        response.body = "";
        response.headers = httplib::Headers{};
        response.success = false;
        response.errorMessage = "Token refresh failed: No token provider";
        context.callback(response);
        return;
    }

    tokenProvider_->refreshAccessToken([this, context](bool success, const std::string& newToken) mutable {
        if (success && !newToken.empty()) {
            // Update access token
            setAccessToken(newToken);

            // Retry the original request
            context.retryCount++;
            executeWithRetry(context);
        } else {
            // Token refresh failed
            NetworkResponse response;
            response.statusCode = 403;
            response.body = "";
            response.headers = httplib::Headers{};
            response.success = false;
            response.errorMessage = "Token refresh failed";
            context.callback(response);
        }
    });
}

NetworkResponse NetworkManager::executeRequest(const RequestContext& context) {
    // Prepare headers
    httplib::Headers headers = context.headers;

    // Inject auth token
    injectAuthToken(headers);

    // Call request interceptor if set
    if (requestInterceptor_) {
        requestInterceptor_(headers);
    }

    // Execute the appropriate request method
    if (context.method == "GET") {
        return doGet(context.path, headers, context.params);
    } else if (context.method == "POST") {
        if (!context.multipartItems.empty()) {
            return doPostMultipart(context.path, context.multipartItems, headers);
        }
        return doPost(context.path, context.body, context.contentType, headers);
    } else if (context.method == "PUT") {
        return doPut(context.path, context.body, context.contentType, headers);
    } else if (context.method == "DELETE") {
        return doDelete(context.path, headers);
    } else if (context.method == "PATCH") {
        return doPatch(context.path, context.body, context.contentType, headers);
    }

    NetworkResponse response;
    response.statusCode = 0;
    response.body = "";
    response.headers = httplib::Headers{};
    response.success = false;
    response.errorMessage = "Unknown method";
    return response;
}

NetworkResponse NetworkManager::processResponse(const httplib::Result& res) const {
    NetworkResponse response;

    if (!res) {
        response.success = false;
        response.statusCode = 0;
        response.errorMessage = "Network error: Could not connect to server";
        return response;
    }

    response.statusCode = res->status;
    response.body = res->body;
    response.headers = res->headers;
    response.success = (res->status >= 200 && res->status < 300);

    if (!response.success) {
        // Get error message based on status code
        response.errorMessage = HttpError::getMessage(res->status);

        // Try to parse error from response body
        if (!res->body.empty()) {
            try {
                json errorJson = json::parse(res->body);
                response.errorMessage = parseErrorMessage(errorJson, response.errorMessage);
            } catch (...) {
                // Keep default error message
            }
        }
    }

    return response;
}

void NetworkManager::executeAsync(std::function<void()> task) {
    std::thread(task).detach();
}

// GET implementations
void NetworkManager::get(const std::string& path, ResponseCallback callback) {
    get(path, httplib::Headers{}, httplib::Params{}, callback);
}

void NetworkManager::get(const std::string& path, const httplib::Headers& headers, ResponseCallback callback) {
    get(path, headers, httplib::Params{}, callback);
}

void NetworkManager::get(const std::string& path, const httplib::Params& params, ResponseCallback callback) {
    get(path, httplib::Headers{}, params, callback);
}

void NetworkManager::get(const std::string& path, const httplib::Headers& headers, const httplib::Params& params, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "GET";
    context.headers = headers;
    context.params = params;
    context.callback = callback;

    executeWithRetry(context);
}

// POST JSON implementations
void NetworkManager::post(const std::string& path, const json& jsonData, ResponseCallback callback) {
    post(path, jsonData, httplib::Headers{}, callback);
}

void NetworkManager::post(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback) {
    std::string jsonStr = jsonData.dump();
    postRaw(path, jsonStr, "application/json", headers, callback);
}

// POST Form implementations
void NetworkManager::postForm(const std::string& path, const httplib::Params& params, ResponseCallback callback) {
    postForm(path, params, httplib::Headers{}, callback);
}

void NetworkManager::postForm(const std::string& path, const httplib::Params& params, const httplib::Headers& headers, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "POST";
    context.headers = headers;
    context.body = httplib::detail::params_to_query_str(params);
    context.contentType = "application/x-www-form-urlencoded";
    context.callback = callback;

    executeWithRetry(context);
}

// POST Multipart implementations
void NetworkManager::postMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, ResponseCallback callback) {
    postMultipart(path, items, httplib::Headers{}, callback);
}

void NetworkManager::postMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, const httplib::Headers& headers, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "POST";
    context.headers = headers;
    context.multipartItems = items;
    context.callback = callback;
    executeWithRetry(context);
}

// POST Raw implementations
void NetworkManager::postRaw(const std::string& path, const std::string& body, const std::string& contentType, ResponseCallback callback) {
    postRaw(path, body, contentType, httplib::Headers{}, callback);
}

void NetworkManager::postRaw(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "POST";
    context.headers = headers;
    context.body = body;
    context.contentType = contentType;
    context.callback = callback;

    executeWithRetry(context);
}

// PUT implementations
void NetworkManager::put(const std::string& path, const json& jsonData, ResponseCallback callback) {
    put(path, jsonData, httplib::Headers{}, callback);
}

void NetworkManager::put(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback) {
    std::string jsonStr = jsonData.dump();
    putRaw(path, jsonStr, "application/json", headers, callback);
}

void NetworkManager::putRaw(const std::string& path, const std::string& body, const std::string& contentType, ResponseCallback callback) {
    putRaw(path, body, contentType, httplib::Headers{}, callback);
}

void NetworkManager::putRaw(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "PUT";
    context.headers = headers;
    context.body = body;
    context.contentType = contentType;
    context.callback = callback;

    executeWithRetry(context);
}

// DELETE implementations
void NetworkManager::del(const std::string& path, ResponseCallback callback) {
    del(path, httplib::Headers{}, callback);
}

void NetworkManager::del(const std::string& path, const httplib::Headers& headers, ResponseCallback callback) {
    RequestContext context;
    context.path = path;
    context.method = "DELETE";
    context.headers = headers;
    context.callback = callback;

    executeWithRetry(context);
}

// PATCH implementations
void NetworkManager::patch(const std::string& path, const json& jsonData, ResponseCallback callback) {
    patch(path, jsonData, httplib::Headers{}, callback);
}

void NetworkManager::patch(const std::string& path, const json& jsonData, const httplib::Headers& headers, ResponseCallback callback) {
    std::string jsonStr = jsonData.dump();
    RequestContext context;
    context.path = path;
    context.method = "PATCH";
    context.headers = headers;
    context.body = jsonStr;
    context.contentType = "application/json";
    context.callback = callback;

    executeWithRetry(context);
}

// Internal request methods
NetworkResponse NetworkManager::doGet(const std::string& path, const httplib::Headers& headers, const httplib::Params& params) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }

    auto mergedHeaders = mergeHeaders(headers);

    std::string fullPath = path;
    if (!params.empty()) {
        fullPath += "?" + httplib::detail::params_to_query_str(params);
    }

    httplib::Result res(nullptr, httplib::Error::Success);
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Get(fullPath.c_str(), mergedHeaders);
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Get(fullPath.c_str(), mergedHeaders);
    }

    return processResponse(res);
}

NetworkResponse NetworkManager::doPost(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }

    auto mergedHeaders = mergeHeaders(headers);

    httplib::Result res{nullptr, httplib::Error::Unknown};
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Post(path.c_str(), mergedHeaders, body, contentType.c_str());
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Post(path.c_str(), mergedHeaders, body, contentType.c_str());
    }

    return processResponse(res);
}

NetworkResponse NetworkManager::doPostMultipart(const std::string& path, const httplib::MultipartFormDataItems& items, const httplib::Headers& headers) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }
    
    auto mergedHeaders = mergeHeaders(headers);
    httplib::Result res{nullptr, httplib::Error::Unknown};
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Post(path.c_str(), mergedHeaders, items);
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Post(path.c_str(), mergedHeaders, items);
    }
    
    auto processedResponse = processResponse(res);      
    return processedResponse;
}

NetworkResponse NetworkManager::doPut(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }

    auto mergedHeaders = mergeHeaders(headers);

    httplib::Result res{nullptr, httplib::Error::Unknown};
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Put(path.c_str(), mergedHeaders, body, contentType.c_str());
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Put(path.c_str(), mergedHeaders, body, contentType.c_str());
    }

    return processResponse(res);
}

NetworkResponse NetworkManager::doDelete(const std::string& path, const httplib::Headers& headers) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }

    auto mergedHeaders = mergeHeaders(headers);

    httplib::Result res{nullptr, httplib::Error::Unknown};
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Delete(path.c_str(), mergedHeaders);
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Delete(path.c_str(), mergedHeaders);
    }

    return processResponse(res);
}

NetworkResponse NetworkManager::doPatch(const std::string& path, const std::string& body, const std::string& contentType, const httplib::Headers& headers) {
    if (!sslClient_ && !httpClient_) {
        NetworkResponse response;
        response.success = false;
        response.errorMessage = "Client not initialized";
        return response;
    }

    auto mergedHeaders = mergeHeaders(headers);

    httplib::Result res{nullptr, httplib::Error::Unknown};
    if (config_.useSSL && sslClient_) {
        res = sslClient_->Patch(path.c_str(), mergedHeaders, body, contentType.c_str());
    } else if (!config_.useSSL && httpClient_) {
        res = httpClient_->Patch(path.c_str(), mergedHeaders, body, contentType.c_str());
    }

    return processResponse(res);
}

// Configuration methods
void NetworkManager::setDefaultHeader(const std::string& key, const std::string& value) {
    // Remove existing header if present
    auto it = config_.defaultHeaders.begin();
    while (it != config_.defaultHeaders.end()) {
        if (it->first == key) {
            it = config_.defaultHeaders.erase(it);
        } else {
            ++it;
        }
    }
    // Add new header
    config_.defaultHeaders.emplace(key, value);
}

void NetworkManager::removeDefaultHeader(const std::string& key) {
    auto it = config_.defaultHeaders.begin();
    while (it != config_.defaultHeaders.end()) {
        if (it->first == key) {
            it = config_.defaultHeaders.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkManager::setTimeout(int connectionTimeout, int readTimeout) {
    config_.connectionTimeout = connectionTimeout;
    config_.readTimeout = readTimeout;

    if (sslClient_) {
        sslClient_->set_connection_timeout(connectionTimeout, 0);
        sslClient_->set_read_timeout(readTimeout, 0);
    }
    if (httpClient_) {
        httpClient_->set_connection_timeout(connectionTimeout, 0);
        httpClient_->set_read_timeout(readTimeout, 0);
    }
}

void NetworkManager::setSSLVerification(bool verify) {
    config_.verifySSL = verify;
    if (config_.useSSL && sslClient_) {
        sslClient_->enable_server_certificate_verification(verify);
    }
}

void NetworkManager::setRetryConfig(int maxRetries, int retryDelayMs, const std::vector<int>& statusCodes) {
    config_.maxRetries = maxRetries;
    config_.retryDelayMs = retryDelayMs;
    config_.retryOnStatusCodes = statusCodes;
}
