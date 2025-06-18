//
//  PureMVCWrapper.h
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Protocol để callback từ C++ về Swift/ObjC
@protocol PureMVCDelegate <NSObject>
@optional
- (void)onCommandExecuted:(NSString *)commandName withData:(nullable id)data;
- (void)onNotificationReceived:(NSString *)notificationName withData:(nullable id)data;
@end

@interface PureMVCWrapper : NSObject

@property (nonatomic, weak) id<PureMVCDelegate> delegate;

// Singleton instance
+ (instancetype)sharedInstance;

// Initialize PureMVC
- (void)initializeFacade;

// Send notifications
- (void)sendNotification:(NSString *)notificationName;
- (void)sendNotification:(NSString *)notificationName withBody:(nullable id)body;

// User actions
- (void)onLoginButtonPressed:(NSString *)username password:(NSString *)password;
- (void)onLogoutButtonPressed;
- (void)onDataRefreshRequested;

@end

NS_ASSUME_NONNULL_END
