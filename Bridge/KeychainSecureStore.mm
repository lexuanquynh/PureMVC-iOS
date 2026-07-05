//
//  KeychainSecureStore.mm
//  PureMVCBridge — platform adapter (iOS/Apple)
//
//  Objective-C++ implementation of core::ISecureStorage over the iOS Keychain.
//  Part of the PureMVCBridge target (links Security.framework); not built by the
//  host CMake unit-test build.
//

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "KeychainSecureStore.hpp"

#include <utility>

namespace core {

namespace {

NSMutableDictionary* baseQuery(const std::string& service, const std::string& key) {
    NSMutableDictionary* query = [NSMutableDictionary dictionary];
    query[(__bridge id)kSecClass] = (__bridge id)kSecClassGenericPassword;
    query[(__bridge id)kSecAttrService] = [NSString stringWithUTF8String:service.c_str()];
    query[(__bridge id)kSecAttrAccount] = [NSString stringWithUTF8String:key.c_str()];
    return query;
}

} // namespace

KeychainSecureStore::KeychainSecureStore(std::string service)
    : service_(std::move(service)) {}

bool KeychainSecureStore::get(const std::string& key, std::string& out) {
    NSMutableDictionary* query = baseQuery(service_, key);
    query[(__bridge id)kSecReturnData] = @YES;
    query[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;

    CFTypeRef result = NULL;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &result);
    if (status != errSecSuccess || result == NULL) {
        return false;
    }

    NSData* data = (__bridge_transfer NSData*)result;
    out.assign(reinterpret_cast<const char*>(data.bytes), data.length);
    return true;
}

bool KeychainSecureStore::set(const std::string& key, const std::string& value) {
    NSMutableDictionary* query = baseQuery(service_, key);
    NSData* data = [NSData dataWithBytes:value.data() length:value.size()];

    // Update the item if it already exists, otherwise add it.
    NSDictionary* attributes = @{ (__bridge id)kSecValueData: data };
    OSStatus status = SecItemUpdate((__bridge CFDictionaryRef)query,
                                    (__bridge CFDictionaryRef)attributes);

    if (status == errSecItemNotFound) {
        NSMutableDictionary* addQuery = [query mutableCopy];
        addQuery[(__bridge id)kSecValueData] = data;
        // Device-only, available after first unlock: not synced to iCloud and
        // not readable while the device is locked.
        addQuery[(__bridge id)kSecAttrAccessible] =
            (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly;
        status = SecItemAdd((__bridge CFDictionaryRef)addQuery, NULL);
    }

    return status == errSecSuccess;
}

bool KeychainSecureStore::remove(const std::string& key) {
    NSMutableDictionary* query = baseQuery(service_, key);
    OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query);
    return status == errSecSuccess || status == errSecItemNotFound;
}

} // namespace core
