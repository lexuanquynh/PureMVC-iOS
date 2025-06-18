//
//  PureMVCConstantsWrapper.m
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#import "PureMVCConstantsWrapper.h"
#include "PureMVCConstants.h"

@implementation PureMVCNotifications

+ (NSString *)loginRequest {
    return [NSString stringWithUTF8String:PureMVCConstants::LOGIN_REQUEST.c_str()];
}

+ (NSString *)loginSuccess {
    return [NSString stringWithUTF8String:PureMVCConstants::LOGIN_SUCCESS.c_str()];
}

+ (NSString *)loginFailed {
    return [NSString stringWithUTF8String:PureMVCConstants::LOGIN_FAILED.c_str()];
}

+ (NSString *)logoutRequest {
    return [NSString stringWithUTF8String:PureMVCConstants::LOGOUT_REQUEST.c_str()];
}

+ (NSString *)logoutSuccess {
    return [NSString stringWithUTF8String:PureMVCConstants::LOGOUT_SUCCESS.c_str()];
}

+ (NSString *)dataRefresh {
    return [NSString stringWithUTF8String:PureMVCConstants::DATA_REFRESH.c_str()];
}

@end
