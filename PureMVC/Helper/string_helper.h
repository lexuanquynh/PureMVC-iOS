//
//  string_helper.h
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#ifndef string_helper_h
#define string_helper_h
#include "json.hpp"

using json = nlohmann::json;

// Parse error from various response formats
std::string parseErrorMessage(const json& response, const std::string& defaultMsg = "Error occurred") {
    try {
        // Try common error field names
        std::vector<std::string> errorFields = {"error", "message", "detail", "error_description", "msg"};

        for (const auto& field : errorFields) {
            if (response.contains(field)) {
                auto value = response[field];
                if (value.is_string()) {
                    std::string msg = value.get<std::string>();
                    if (!msg.empty()) {
                        return msg;
                    }
                }
            }
        }

        // Check for nested error object
        if (response.contains("error") && response["error"].is_object()) {
            json errorObj = response["error"];
            for (const auto& field : {"message", "detail", "description"}) {
                if (errorObj.contains(field) && errorObj[field].is_string()) {
                    std::string msg = errorObj[field].get<std::string>();
                    if (!msg.empty()) {
                        return msg;
                    }
                }
            }
        }

    } catch (...) {
        // Ignore parsing errors
    }

    return defaultMsg;
}

// Safe string helper for C++
//std::string safeString(const std::string& str, const std::string& defaultValue = "") {
//    return str.empty() ? defaultValue : str;
//}


#endif /* string_helper_h */
