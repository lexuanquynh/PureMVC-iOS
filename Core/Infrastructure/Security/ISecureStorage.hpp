//
//  ISecureStorage.hpp
//  PureMVC Core — Infrastructure port
//
//  Minimal secure key/value store. The iOS Keychain sits behind this; tests use
//  an in-memory fake. Keeps the token-store logic independent of the platform.
//

#ifndef PUREMVC_CORE_ISECURE_STORAGE_HPP
#define PUREMVC_CORE_ISECURE_STORAGE_HPP

#include <string>

namespace core {

class ISecureStorage {
public:
    // Returns true and fills 'out' when 'key' exists; false when absent.
    virtual bool get(const std::string& key, std::string& out) = 0;

    // Stores (or replaces) the value for 'key'. Returns true on success.
    virtual bool set(const std::string& key, const std::string& value) = 0;

    // Removes 'key'. Returns true if it no longer exists afterwards.
    virtual bool remove(const std::string& key) = 0;

    virtual ~ISecureStorage() = default;
};

} // namespace core

#endif // PUREMVC_CORE_ISECURE_STORAGE_HPP
