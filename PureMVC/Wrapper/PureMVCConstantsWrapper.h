//
//  PureMVCConstantsWrapper.h
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Objective-C wrapper for PureMVC constants
@interface PureMVCNotifications : NSObject

// Notifications
@property (class, readonly) NSString *loginRequest;
@property (class, readonly) NSString *loginSuccess;
@property (class, readonly) NSString *loginFailed;
@property (class, readonly) NSString *logoutRequest;
@property (class, readonly) NSString *logoutSuccess;
@property (class, readonly) NSString *dataRefresh;

@end

NS_ASSUME_NONNULL_END
