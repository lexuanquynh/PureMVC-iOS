//
//  HttpTypes.hpp
//  PureMVC Core — Infrastructure
//
//  Transport-neutral HTTP value objects. No httplib, no platform types, so the
//  repositories and their tests never depend on the concrete client.
//

#ifndef PUREMVC_CORE_HTTP_TYPES_HPP
#define PUREMVC_CORE_HTTP_TYPES_HPP

#include <map>
#include <string>

namespace core {

struct HttpRequest {
    std::string method;                       // "GET", "POST", ...
    std::string path;                         // e.g. "/api/v1/auth/login"
    std::map<std::string, std::string> headers;
    std::string body;
    std::string contentType = "application/json";
};

struct HttpResponse {
    int status = 0;                           // HTTP status; 0 when unreachable
    std::string body;
    std::map<std::string, std::string> headers;

    // Set when the request never produced an HTTP status (DNS failure, timeout,
    // TLS error, ...). Distinguishes "couldn't connect" from "server said 4xx".
    bool transportError = false;
    std::string transportErrorMessage;

    bool ok() const {
        return !transportError && status >= 200 && status < 300;
    }
};

} // namespace core

#endif // PUREMVC_CORE_HTTP_TYPES_HPP
