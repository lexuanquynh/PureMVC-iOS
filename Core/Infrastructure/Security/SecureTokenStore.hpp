//
//  SecureTokenStore.hpp
//  PureMVC Core — Infrastructure
//
//  ITokenStore implemented over any ISecureStorage. Pure C++: it owns the
//  serialization and load/save/clear semantics, so it is fully host-testable.
//  On iOS, compose it with KeychainSecureStore for a Keychain-backed store.
//

#ifndef PUREMVC_CORE_SECURE_TOKEN_STORE_HPP
#define PUREMVC_CORE_SECURE_TOKEN_STORE_HPP

#include <string>
#include "Domain/Ports/ITokenStore.hpp"
#include "Infrastructure/Security/ISecureStorage.hpp"

namespace core {

class SecureTokenStore : public ITokenStore {
public:
    explicit SecureTokenStore(ISecureStorage& storage,
                              std::string key = "com.puremvc.auth.tokens");

    Token load() override;
    void save(const Token& token) override;
    void clear() override;

private:
    ISecureStorage& storage_;
    std::string key_;
};

} // namespace core

#endif // PUREMVC_CORE_SECURE_TOKEN_STORE_HPP
