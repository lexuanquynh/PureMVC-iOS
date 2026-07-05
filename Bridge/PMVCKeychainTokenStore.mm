//
//  PMVCKeychainTokenStore.mm
//  PureMVCBridge
//
//  ObjC++ implementation: bridges the pure-ObjC API to the C++ Core.
//

#import "PureMVCBridge/PMVCKeychainTokenStore.h"

#include "KeychainSecureStore.hpp"
#include "Infrastructure/Security/SecureTokenStore.hpp"

#include <memory>
#include <string>

@implementation PMVCTokenPair
@end

@implementation PMVCKeychainTokenStore {
    std::unique_ptr<core::KeychainSecureStore> _keychain;
    std::unique_ptr<core::SecureTokenStore> _store;
}

- (instancetype)initWithService:(NSString *)service {
    if (self = [super init]) {
        _keychain = std::unique_ptr<core::KeychainSecureStore>(
            new core::KeychainSecureStore(service.UTF8String ? service.UTF8String : ""));
        _store = std::unique_ptr<core::SecureTokenStore>(
            new core::SecureTokenStore(*_keychain));
    }
    return self;
}

- (void)saveAccessToken:(NSString *)accessToken refreshToken:(NSString *)refreshToken {
    core::Token token;
    token.accessToken = accessToken.UTF8String ? accessToken.UTF8String : "";
    token.refreshToken = refreshToken.UTF8String ? refreshToken.UTF8String : "";
    _store->save(token);
}

- (nullable PMVCTokenPair *)load {
    core::Token token = _store->load();
    if (token.accessToken.empty() && token.refreshToken.empty()) {
        return nil;
    }
    PMVCTokenPair *pair = [PMVCTokenPair new];
    pair.accessToken = [NSString stringWithUTF8String:token.accessToken.c_str()];
    pair.refreshToken = [NSString stringWithUTF8String:token.refreshToken.c_str()];
    return pair;
}

- (void)clear {
    _store->clear();
}

@end
