//
//  SecureTokenStore.cpp
//  PureMVC Core — Infrastructure
//

#include "Infrastructure/Security/SecureTokenStore.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace core {

SecureTokenStore::SecureTokenStore(ISecureStorage& storage, std::string key)
    : storage_(storage), key_(std::move(key)) {}

Token SecureTokenStore::load() {
    std::string blob;
    if (!storage_.get(key_, blob)) {
        return Token{};
    }
    try {
        json j = json::parse(blob);
        Token token;
        token.accessToken = j.value("access_token", std::string());
        token.refreshToken = j.value("refresh_token", std::string());
        return token;
    } catch (...) {
        // Corrupt / unexpected payload — behave as if nothing is stored.
        return Token{};
    }
}

void SecureTokenStore::save(const Token& token) {
    json j = {
        {"access_token", token.accessToken},
        {"refresh_token", token.refreshToken},
    };
    storage_.set(key_, j.dump());
}

void SecureTokenStore::clear() {
    storage_.remove(key_);
}

} // namespace core
