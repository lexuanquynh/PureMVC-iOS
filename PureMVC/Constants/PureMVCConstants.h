//
//  PureMVCConstants.h
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#ifndef PUREMVC_CONSTANTS_H
#define PUREMVC_CONSTANTS_H

#include <string>

namespace PureMVCConstants {
    // Notification names
    const std::string LOGIN_REQUEST = "LOGIN_REQUEST";
    const std::string LOGIN_SUCCESS = "LOGIN_SUCCESS";
    const std::string LOGIN_FAILED = "LOGIN_FAILED";
    const std::string LOGOUT_REQUEST = "LOGOUT_REQUEST";
    const std::string LOGOUT_SUCCESS = "LOGOUT_SUCCESS";
    const std::string DATA_REFRESH = "DATA_REFRESH";

    // Facade name
    const std::string FACADE_NAME = "MyApp";
}

// define error
namespace PureMVCConstants {
    const std::string ERROR_BAD_REQUEST = "Bad request";
    const std::string ERROR_INVALID_CREDENTIALS = "Invalid credentials";
    const std::string ERROR_ACCESS_FORBIDDEN = "Access forbidden";
    const std::string ERROR_ENDPOINT_NOT_FOUND = "Login endpoint not found";
    const std::string ERROR_TOO_MANY_ATTEMPTS = "Too many login attempts";
    const std::string ERROR_SERVER_ERROR = "Server error";
}

#endif // PUREMVC_CONSTANTS_H
