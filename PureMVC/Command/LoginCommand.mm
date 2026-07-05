//
//  LoginCommand.mm
//  PureMVC
//
//  Created by Prank on 17/6/25.
//
//  Routes login through the Core module (via the PureMVCBridge Swift Package):
//  HttplibHttpClient (TLS verify + pinning) + AuthRepository + LoginUseCase +
//  Keychain-backed SecureTokenStore. The old NetworkManager/UserProxy login path
//  is no longer used here.
//

#import <Foundation/Foundation.h>
#import <PureMVCBridge/PMVCAuthClient.h>

#include "LoginCommand.h"
#include "PureMVC/Patterns/Facade/Facade.hpp"

// Shared, process-lifetime auth client. Host is a placeholder for the demo; add
// real SPKI pins here (pinnedSPKIHashes) when pointing at a real backend.
static PMVCAuthClient *SharedAuthClient() {
    static PMVCAuthClient *client = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        client = [[PMVCAuthClient alloc] initWithHost:@"sample.com"
                                                 port:443
                                     pinnedSPKIHashes:nil];
    });
    return client;
}

void LoginCommand::execute(INotification const& notification) {
    NSLog(@"LoginCommand::execute called");

    const void *bodyPtr = notification.getBody();
    NSDictionary *loginData = (__bridge NSDictionary *)bodyPtr;
    if (!loginData) return;

    NSString *username = loginData[@"username"];
    NSString *password = loginData[@"password"];

    IFacade *facadePtr = &getFacade();

    [SharedAuthClient() loginWithEmail:(username ?: @"")
                             password:(password ?: @"")
                           completion:^(BOOL success, NSString *_Nullable message) {
        // PMVCAuthClient always delivers on the main queue.
        if (success) {
            facadePtr->sendNotification("LOGIN_SUCCESS");
        } else {
            NSDictionary *errorData = @{ @"message": (message ?: @"Login failed") };
            void *errorPtr = (__bridge void *)errorData;
            facadePtr->sendNotification("LOGIN_FAILED", errorPtr);
        }
    }];
}
