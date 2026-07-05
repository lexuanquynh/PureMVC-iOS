//
//  PMVCAuthClient.h
//  PureMVCBridge — public, Swift-facing API
//
//  Pure Objective-C façade over the C++ Core auth flow (HTTP client + repository
//  + use case + Keychain token store). The app talks to this; it never includes
//  Core C++ headers.
//

#import <Foundation/Foundation.h>
#import "PMVCKeychainTokenStore.h"

NS_ASSUME_NONNULL_BEGIN

@interface PMVCAuthClient : NSObject

/// @param host  API host only, e.g. "api.example.com"
/// @param port  usually 443
/// @param pinnedSPKIHashes  optional base64(SHA-256(SPKI)) pins; nil/empty = no pinning
- (instancetype)initWithHost:(NSString *)host
                        port:(NSInteger)port
            pinnedSPKIHashes:(nullable NSArray<NSString *> *)pinnedSPKIHashes
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

/// Performs login; `completion` is always invoked on the main queue.
- (void)loginWithEmail:(NSString *)email
              password:(NSString *)password
            completion:(void (^)(BOOL success, NSString * _Nullable message))completion;

/// Clears the stored session tokens.
- (void)logout;

/// The currently stored tokens, or nil if none.
- (nullable PMVCTokenPair *)currentTokens;

@end

NS_ASSUME_NONNULL_END
