//
//  http_errors.h
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#ifndef http_errors_h
#define http_errors_h
#include <string>
#include <unordered_map>

struct HttpError {
    static const std::unordered_map<int, std::string> statusMessages;

    static std::string getMessage(int statusCode) {
        auto it = statusMessages.find(statusCode);
        if (it != statusMessages.end()) {
            return it->second;
        }
        return "Error " + std::to_string(statusCode);
    }
};

const std::unordered_map<int, std::string> HttpError::statusMessages = {
    {400, "Bad request"},
    {401, "Invalid credentials"},
    {403, "Access forbidden"},
    {404, "Login endpoint not found"},
    {429, "Too many login attempts"},
    {500, "Server error"},
};


#endif /* http_errors_h */
