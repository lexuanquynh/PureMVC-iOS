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

#include "Infrastructure/Security/Base64.hpp"
#include "Infrastructure/Security/CertificatePinner.hpp"

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#endif

namespace core {
namespace {

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
// base64(SHA-256(SubjectPublicKeyInfo DER)) for the given certificate, or "" on
// failure. This is the value compared against the configured pins.
std::string computeSpkiSha256Base64(X509* cert) {
    X509_PUBKEY* pubkey = X509_get_X509_PUBKEY(cert); // internal pointer, do not free
    unsigned char* der = nullptr;
    int len = i2d_X509_PUBKEY(pubkey, &der);
    if (len <= 0 || der == nullptr) {
        return std::string();
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(der, static_cast<size_t>(len), hash);
    OPENSSL_free(der);
    return base64Encode(hash, SHA256_DIGEST_LENGTH);
}
#endif

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
        if (!config.caCertPath.empty()) {
            client.set_ca_cert_path(config.caCertPath);
        }

        CertificatePinner pinner(config.pinnedSpkiSha256Base64);
        if (pinner.enabled()) {
            client.set_server_certificate_verifier(
                [pinner](SSL* ssl) -> httplib::SSLVerifierResponse {
                    // SSL_get1_peer_certificate is OpenSSL 3.0+; 1.1.1 (e.g. the
                    // Android NDK prefab) uses SSL_get_peer_certificate. Both
                    // return an owned cert that must be X509_free'd.
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
                    X509* cert = SSL_get1_peer_certificate(ssl);
#else
                    X509* cert = SSL_get_peer_certificate(ssl);
#endif
                    if (cert == nullptr) {
                        return httplib::SSLVerifierResponse::CertificateRejected;
                    }
                    const std::string pin = computeSpkiSha256Base64(cert);
                    X509_free(cert);
                    if (pin.empty() || !pinner.isTrusted(pin)) {
                        return httplib::SSLVerifierResponse::CertificateRejected;
                    }
                    // Pin matches; defer to the built-in chain/host verifier.
                    return httplib::SSLVerifierResponse::NoDecisionMade;
                });
        }

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
