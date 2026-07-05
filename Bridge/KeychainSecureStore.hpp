//
//  KeychainSecureStore.hpp
//  PureMVC Core — Infrastructure (iOS/Apple only)
//
//  ISecureStorage backed by the iOS Keychain. The declaration is pure C++ so it
//  can be referenced from C++ code; the implementation (KeychainSecureStore.mm)
//  is Objective-C++ and links Security.framework, so it is compiled into the
//  Apple app target, NOT the host CMake build.
//

#ifndef PUREMVC_CORE_KEYCHAIN_SECURE_STORE_HPP
#define PUREMVC_CORE_KEYCHAIN_SECURE_STORE_HPP

#include <string>
#include "Infrastructure/Security/ISecureStorage.hpp"

namespace core {

class KeychainSecureStore : public ISecureStorage {
public:
    // 'service' becomes the Keychain item's kSecAttrService; each key maps to a
    // kSecAttrAccount under it.
    explicit KeychainSecureStore(std::string service = "com.puremvc.auth");

    bool get(const std::string& key, std::string& out) override;
    bool set(const std::string& key, const std::string& value) override;
    bool remove(const std::string& key) override;

private:
    std::string service_;
};

} // namespace core

#endif // PUREMVC_CORE_KEYCHAIN_SECURE_STORE_HPP
