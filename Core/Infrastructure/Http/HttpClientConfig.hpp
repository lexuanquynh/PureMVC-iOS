//
//  HttpClientConfig.hpp
//  PureMVC Core — Infrastructure
//
//  Connection settings for a concrete HTTP client. Platform-neutral.
//

#ifndef PUREMVC_CORE_HTTP_CLIENT_CONFIG_HPP
#define PUREMVC_CORE_HTTP_CLIENT_CONFIG_HPP

#include <map>
#include <string>
#include <vector>

namespace core {

struct HttpClientConfig {
    std::string host;                 // host only, e.g. "api.example.com"
    int port = 443;
    bool useSSL = true;
    bool verifySSL = true;            // server certificate chain verification (on)
    int connectionTimeoutSec = 10;
    int readTimeoutSec = 10;
    std::map<std::string, std::string> defaultHeaders;

    // Optional path to a CA bundle (PEM). Empty => use the system trust store.
    std::string caCertPath;

    // Optional public-key pins: base64(SHA-256(SubjectPublicKeyInfo DER)).
    // Empty => pinning disabled (chain verification still applies). Any match
    // against the leaf certificate's SPKI is accepted (supports key rotation
    // by listing current + backup pins).
    std::vector<std::string> pinnedSpkiSha256Base64;
};

} // namespace core

#endif // PUREMVC_CORE_HTTP_CLIENT_CONFIG_HPP
