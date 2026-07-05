//
//  HttpError.hpp
//  PureMVC Core — Infrastructure
//
//  Default human-readable messages per HTTP status. Mirrors the behavior of
//  the legacy http_errors.h so error text stays consistent across the migration.
//

#ifndef PUREMVC_CORE_HTTP_ERROR_HPP
#define PUREMVC_CORE_HTTP_ERROR_HPP

#include <string>

namespace core {

inline std::string httpStatusMessage(int status) {
    switch (status) {
        case 400: return "Bad request";
        case 401: return "Invalid credentials";
        case 403: return "Access forbidden";
        case 404: return "Login endpoint not found";
        case 429: return "Too many login attempts";
        case 500: return "Server error";
        default:  return "Error " + std::to_string(status);
    }
}

} // namespace core

#endif // PUREMVC_CORE_HTTP_ERROR_HPP
