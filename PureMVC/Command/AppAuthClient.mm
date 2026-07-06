//
//  AppAuthClient.mm
//  PureMVC
//

#import <Foundation/Foundation.h>
#import "AppAuthClient.h"

PMVCAuthClient *AppSharedAuthClient(void) {
    static PMVCAuthClient *client = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        client = [[PMVCAuthClient alloc] initWithHost:@"sample.com"
                                                 port:443
                                     pinnedSPKIHashes:nil];
    });
    return client;
}
