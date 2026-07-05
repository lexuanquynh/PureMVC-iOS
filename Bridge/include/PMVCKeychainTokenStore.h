//
//  PMVCKeychainTokenStore.h
//  PureMVCBridge — public, Swift-facing API
//
//  Pure Objective-C header (no C++), so Swift can import it directly. Wraps the
//  C++ SecureTokenStore backed by the iOS Keychain.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PMVCTokenPair : NSObject
@property (nonatomic, copy) NSString *accessToken;
@property (nonatomic, copy) NSString *refreshToken;
@end

@interface PMVCKeychainTokenStore : NSObject

- (instancetype)initWithService:(NSString *)service NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

- (void)saveAccessToken:(NSString *)accessToken refreshToken:(NSString *)refreshToken;

// Returns nil when nothing is stored.
- (nullable PMVCTokenPair *)load;

- (void)clear;

@end

NS_ASSUME_NONNULL_END
