//
//  AuthRepository.cpp
//  PureMVC Core — Infrastructure
//

#include "Infrastructure/Auth/AuthRepository.hpp"
#include "Infrastructure/Http/HttpError.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace core {
namespace {

// Pulls a human-readable error out of a JSON error body, trying the field names
// backends commonly use; falls back to the supplied default. Mirrors the legacy
// parseErrorMessage() so behavior is preserved across the migration.
std::string extractErrorMessage(const std::string& body, const std::string& fallback) {
    if (body.empty()) {
        return fallback;
    }
    try {
        json j = json::parse(body);

        const char* fields[] = {"error", "message", "detail", "error_description", "msg"};
        for (const char* field : fields) {
            if (j.contains(field) && j[field].is_string()) {
                std::string msg = j[field].get<std::string>();
                if (!msg.empty()) {
                    return msg;
                }
            }
        }

        // Nested { "error": { "message": ... } } shape.
        if (j.contains("error") && j["error"].is_object()) {
            const json& err = j["error"];
            for (const char* field : {"message", "detail", "description"}) {
                if (err.contains(field) && err[field].is_string()) {
                    std::string msg = err[field].get<std::string>();
                    if (!msg.empty()) {
                        return msg;
                    }
                }
            }
        }
    } catch (...) {
        // Non-JSON body — keep the fallback.
    }
    return fallback;
}

} // namespace

AuthRepository::AuthRepository(IHttpClient& httpClient, std::string loginPath)
    : httpClient_(httpClient), loginPath_(std::move(loginPath)) {}

void AuthRepository::login(const LoginCredentials& credentials, LoginCallback callback) {
    json requestBody = {
        {"email", credentials.email},
        {"password", credentials.password},
    };

    HttpRequest request;
    request.method = "POST";
    request.path = loginPath_;
    request.contentType = "application/json";
    request.body = requestBody.dump();

    // The backend does not echo the username; carry it from the request so the
    // resulting session is complete.
    const std::string email = credentials.email;

    httpClient_.send(request, [callback, email](const HttpResponse& response) {
        if (response.ok()) {
            try {
                json j = json::parse(response.body);
                AuthSession session;
                session.user.username = email;
                session.user.isVerified = j.value("is_verify", false);
                session.token.accessToken = j.value("access_token", std::string());
                session.token.refreshToken = j.value("refresh_token", std::string());
                callback(true, session, DomainError{});
            } catch (const std::exception& e) {
                callback(false, AuthSession{},
                         DomainError(std::string("Failed to parse response: ") + e.what()));
            }
            return;
        }

        if (response.transportError) {
            std::string msg = response.transportErrorMessage.empty()
                ? "Network error: Could not connect to server"
                : response.transportErrorMessage;
            callback(false, AuthSession{}, DomainError(msg, 0));
            return;
        }

        std::string fallback = httpStatusMessage(response.status);
        std::string msg = extractErrorMessage(response.body, fallback);
        callback(false, AuthSession{}, DomainError(msg, response.status));
    });
}

} // namespace core
