//
//  AppAuthClient.h
//  PureMVC
//
//  Single composition point for the app's Core-backed auth client, shared by
//  LoginCommand and LogoutCommand.
//

#import <PureMVCBridge/PMVCAuthClient.h>

/// Process-lifetime shared auth client. Host is a placeholder for the demo; add
/// real SPKI pins (pinnedSPKIHashes) here when pointing at a real backend.
PMVCAuthClient *AppSharedAuthClient(void);
