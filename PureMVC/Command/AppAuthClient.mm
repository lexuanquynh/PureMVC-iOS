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
        // Demo uses mock data so login succeeds offline. For a real backend swap
        // this for -initWithHost:port:pinnedSPKIHashes:.
        client = [[PMVCAuthClient alloc] initWithMockData];
    });
    return client;
}
