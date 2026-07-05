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

namespace core {

struct HttpClientConfig {
    std::string host;                 // host only, e.g. "api.example.com"
    int port = 443;
    bool useSSL = true;
    bool verifySSL = true;            // server certificate verification
    int connectionTimeoutSec = 10;
    int readTimeoutSec = 10;
    std::map<std::string, std::string> defaultHeaders;
};

} // namespace core

#endif // PUREMVC_CORE_HTTP_CLIENT_CONFIG_HPP
