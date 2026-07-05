//
//  PMVCAuthClient.mm
//  PureMVCBridge
//
//  Composes the C++ Core auth stack and marshals between ObjC and C++.
//

#import "PMVCAuthClient.h"

#include "KeychainSecureStore.hpp"
#include "Infrastructure/Http/HttpClientConfig.hpp"
#include "Infrastructure/Http/HttplibHttpClient.hpp"
#include "Infrastructure/Concurrency/ThreadExecutor.hpp"
#include "Infrastructure/Auth/AuthRepository.hpp"
#include "Infrastructure/Security/SecureTokenStore.hpp"
#include "Domain/UseCases/LoginUseCase.hpp"

#include <memory>
#include <string>
#include <vector>

namespace {

std::string toStd(NSString *s) {
    return s.UTF8String ? std::string(s.UTF8String) : std::string();
}

NSString *toNS(const std::string &s) {
    NSString *out = [NSString stringWithUTF8String:s.c_str()];
    return out ?: @"";
}

} // namespace

@implementation PMVCAuthClient {
    // Declaration order == construction order; ARC destroys C++ ivars in reverse,
    // which keeps every reference valid (login → store/repo → keychain/client → executor).
    std::unique_ptr<core::ThreadExecutor> _executor;
    std::unique_ptr<core::HttplibHttpClient> _client;
    std::unique_ptr<core::AuthRepository> _repository;
    std::unique_ptr<core::KeychainSecureStore> _keychain;
    std::unique_ptr<core::SecureTokenStore> _store;
    std::unique_ptr<core::LoginUseCase> _login;
}

- (instancetype)initWithHost:(NSString *)host
                        port:(NSInteger)port
            pinnedSPKIHashes:(nullable NSArray<NSString *> *)pinnedSPKIHashes {
    if (self = [super init]) {
        core::HttpClientConfig config;
        config.host = toStd(host);
        config.port = (int)port;
        config.useSSL = true;
        config.verifySSL = true;
        for (NSString *pin in pinnedSPKIHashes) {
            config.pinnedSpkiSha256Base64.push_back(toStd(pin));
        }

        _executor = std::unique_ptr<core::ThreadExecutor>(new core::ThreadExecutor());
        _client = std::unique_ptr<core::HttplibHttpClient>(
            new core::HttplibHttpClient(config, *_executor));
        _repository = std::unique_ptr<core::AuthRepository>(
            new core::AuthRepository(*_client, "/api/v1/auth/login"));
        _keychain = std::unique_ptr<core::KeychainSecureStore>(
            new core::KeychainSecureStore("com.puremvc.auth"));
        _store = std::unique_ptr<core::SecureTokenStore>(
            new core::SecureTokenStore(*_keychain));
        _login = std::unique_ptr<core::LoginUseCase>(
            new core::LoginUseCase(*_repository, *_store));
    }
    return self;
}

- (void)loginWithEmail:(NSString *)email
              password:(NSString *)password
            completion:(void (^)(BOOL, NSString * _Nullable))completion {
    core::LoginCredentials creds;
    creds.email = toStd(email);
    creds.password = toStd(password);

    void (^done)(BOOL, NSString *) = [completion copy];
    // Capture self so the client (and its C++ members) outlives the in-flight
    // request; ARC manages ObjC pointers captured by the C++ lambda.
    PMVCAuthClient *retained = self;

    _login->execute(creds, [done, retained](bool success, const std::string &message) {
        NSString *msg = toNS(message);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (done) {
                done(success ? YES : NO, msg);
            }
            (void)retained;
        });
    });
}

- (void)logout {
    _store->clear();
}

- (nullable PMVCTokenPair *)currentTokens {
    core::Token token = _store->load();
    if (token.accessToken.empty() && token.refreshToken.empty()) {
        return nil;
    }
    PMVCTokenPair *pair = [PMVCTokenPair new];
    pair.accessToken = toNS(token.accessToken);
    pair.refreshToken = toNS(token.refreshToken);
    return pair;
}

@end
