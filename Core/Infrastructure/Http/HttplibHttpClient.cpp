//
//  HttplibHttpClient.cpp
//  PureMVC Core — Infrastructure
//
//  NOTE: To enable HTTPS, the build must define CPPHTTPLIB_OPENSSL_SUPPORT
//  before this include and link OpenSSL (done in the iOS target). The host test
//  build compiles plain HTTP only; SSL paths are guarded out below.
//

#include "Infrastructure/Http/HttplibHttpClient.hpp"

#include <utility>
#include <httplib.h>

namespace core {
namespace {

httplib::Headers mergeHeaders(const std::map<std::string, std::string>& defaults,
                              const HttpRequest& request) {
    httplib::Headers headers;
    for (const auto& kv : defaults) {
        headers.emplace(kv.first, kv.second);
    }
    // Per-request headers take precedence.
    for (const auto& kv : request.headers) {
        headers.erase(kv.first);
        headers.emplace(kv.first, kv.second);
    }
    return headers;
}

HttpResponse toResponse(const httplib::Result& result) {
    HttpResponse response;
    if (!result) {
        response.transportError = true;
        response.status = 0;
        response.transportErrorMessage =
            "Network error: " + httplib::to_string(result.error());
        return response;
    }
    response.status = result->status;
    response.body = result->body;
    for (const auto& header : result->headers) {
        response.headers[header.first] = header.second;
    }
    return response;
}

// SSLClient and Client share the same request API, so the dispatch is generic.
template <typename Client>
HttpResponse dispatch(Client& client, const HttpRequest& request,
                      const httplib::Headers& headers) {
    const std::string& method = request.method;
    const char* path = request.path.c_str();
    const char* contentType = request.contentType.c_str();

    if (method == "GET") {
        return toResponse(client.Get(path, headers));
    }
    if (method == "POST") {
        return toResponse(client.Post(path, headers, request.body, contentType));
    }
    if (method == "PUT") {
        return toResponse(client.Put(path, headers, request.body, contentType));
    }
    if (method == "PATCH") {
        return toResponse(client.Patch(path, headers, request.body, contentType));
    }
    if (method == "DELETE") {
        return toResponse(client.Delete(path, headers));
    }

    HttpResponse response;
    response.transportError = true;
    response.transportErrorMessage = "Unsupported HTTP method: " + method;
    return response;
}

template <typename Client>
void configure(Client& client, const HttpClientConfig& config) {
    client.set_connection_timeout(config.connectionTimeoutSec, 0);
    client.set_read_timeout(config.readTimeoutSec, 0);
}

HttpResponse perform(const HttpClientConfig& config, const HttpRequest& request) {
    const httplib::Headers headers = mergeHeaders(config.defaultHeaders, request);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (config.useSSL) {
        httplib::SSLClient client(config.host, config.port);
        client.enable_server_certificate_verification(config.verifySSL);
        configure(client, config);
        return dispatch(client, request, headers);
    }
#else
    if (config.useSSL) {
        HttpResponse response;
        response.transportError = true;
        response.transportErrorMessage = "SSL not supported in this build";
        return response;
    }
#endif

    httplib::Client client(config.host, config.port);
    configure(client, config);
    return dispatch(client, request, headers);
}

} // namespace

HttplibHttpClient::HttplibHttpClient(HttpClientConfig config, IExecutor& executor)
    : config_(std::move(config)), executor_(executor) {}

void HttplibHttpClient::send(const HttpRequest& request, Callback callback) {
    const HttpClientConfig config = config_;
    HttpRequest requestCopy = request;
    executor_.run([config, requestCopy, callback]() {
        callback(perform(config, requestCopy));
    });
}

} // namespace core
