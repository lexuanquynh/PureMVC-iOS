//
//  LoginCommand.m
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#import <Foundation/Foundation.h>
#include "LoginCommand.h"
#include "UserProxy.h"
#include "PureMVC/Patterns/Facade/Facade.hpp"
#include "StringHelper.h"

void LoginCommand::execute(INotification const& notification) {
    NSLog(@"LoginCommand::execute called");
    
    const void* bodyPtr = notification.getBody();
    NSDictionary* loginData = (__bridge NSDictionary*)bodyPtr;
    if (!loginData) return;
    
    NSString* username = loginData[@"username"];
    NSString* password = loginData[@"password"];
    
    IFacade& facade = getFacade();
    try {
        IProxy& baseProxy = facade.retrieveProxy(UserProxy::NAME);
        UserProxy* userProxy = dynamic_cast<UserProxy*>(&baseProxy);
        
        if (userProxy) {
            facade.sendNotification("LOGIN_LOADING");
            
            // Capture facade pointer
            IFacade* facadePtr = &facade;
            
            // Copy username to avoid it being deallocated
            NSString* usernameCopy = [username copy];
            
            userProxy->login(
                safeStdStringFromNSString(username),
                safeStdStringFromNSString(password),
                [facadePtr, usernameCopy](bool success, const std::string& message) {
                    // Capture message by value to ensure it's valid
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (!facadePtr) return;
                        
                        if (success) {
                            NSLog(@"Login successful for user: %@", usernameCopy);
                            facadePtr->sendNotification("LOGIN_SUCCESS");
                        } else {
                            // Create NSString immediately to avoid string lifetime issues
                            NSString* errorMessage = nil;
                            
                            if (message.empty()) {
                                errorMessage = @"Login failed";
                                NSLog(@"Login failed: No error message");
                            } else {
                                // Create NSString from std::string safely
                                @try {
                                    errorMessage = [[NSString alloc] initWithBytes:message.data()
                                                                             length:message.length()
                                                                           encoding:NSUTF8StringEncoding];
                                    if (!errorMessage) {
                                        errorMessage = @"Login failed";
                                    }
                                    NSLog(@"Login failed: %@", errorMessage);
                                } @catch (NSException *exception) {
                                    errorMessage = @"Login failed";
                                    NSLog(@"Login failed: Exception creating error message");
                                }
                            }
                            
                            NSDictionary* errorData = @{
                                @"message": errorMessage
                            };
                            
                            void* errorPtr = (__bridge void*)errorData;
                            facadePtr->sendNotification("LOGIN_FAILED", errorPtr);
                        }
                    });
                }
            );
        } else {
            NSLog(@"Failed to cast to UserProxy");
            NSDictionary* errorData = @{@"message": @"Internal error: Invalid proxy"};
            void* errorPtr = (__bridge void*)errorData;
            facade.sendNotification("LOGIN_FAILED", errorPtr);
        }
    } catch (...) {
        NSLog(@"Exception in LoginCommand");
        NSDictionary* errorData = @{@"message": @"Internal error occurred"};
        void* errorPtr = (__bridge void*)errorData;
        facade.sendNotification("LOGIN_FAILED", errorPtr);
    }
}

/*
void LoginCommand::execute(INotification const& notification) {
    NSLog(@"LoginCommand::execute called");
    
    // Get login data from notification
    const void* bodyPtr = notification.getBody();
    NSDictionary* loginData = (__bridge NSDictionary*)bodyPtr;
    if (!loginData) return;
    
    NSString* username = loginData[@"username"];
    NSString* password = loginData[@"password"];
    
    // Get UserProxy
    IFacade& facade = getFacade();
    try {
        IProxy& baseProxy = facade.retrieveProxy(UserProxy::NAME);
        UserProxy* userProxy = dynamic_cast<UserProxy*>(&baseProxy);
        
        if (userProxy) {
            // Show loading notification
            facade.sendNotification("LOGIN_LOADING");
            
            // Capture facade reference for use in callback
            IFacade* facadePtr = &facade;
            
            // Call async login
            userProxy->login(
                [username UTF8String],
                [password UTF8String],
                             [facadePtr, username](bool success, const std::string& message) {
                                 // This callback runs on background thread
                                 
                                 // Ensure we have a valid facade reference
                                 if (!facadePtr) return;
                                 
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                     if (success) {
                                         NSLog(@"Login successful for user: %@", username);
                                         facadePtr->sendNotification("LOGIN_SUCCESS");
                                     } else {
                                         NSLog(@"Login failed: %s", message.empty() ? "Unknown error" : message.c_str());
                                         
                                         // Create error message safely
                                         NSString* errorMessage = @"Login failed";
                                         
                                         if (!message.empty()) {
                                             @try {
                                                 errorMessage = [NSString stringWithUTF8String:message.c_str()];
                                                 if (!errorMessage) {
                                                     errorMessage = @"Login failed";
                                                 }
                                             } @catch (NSException *exception) {
                                                 NSLog(@"Exception creating error message: %@", exception);
                                                 errorMessage = @"Login failed";
                                             }
                                         }
                                         
                                         // Create error data dictionary
                                         NSDictionary* errorData = @{
                                             @"message": errorMessage
                                         };
                                         
                                         void* errorPtr = (__bridge void*)errorData;
                                         facadePtr->sendNotification("LOGIN_FAILED", errorPtr);
                                     }
                                 });
                             }
                             );
        } else {
            NSLog(@"Failed to cast to UserProxy");
            
            // Send error notification with default message
            NSDictionary* errorData = @{
                @"message": @"Internal error: Invalid proxy"
            };
            void* errorPtr = (__bridge void*)errorData;
            facade.sendNotification("LOGIN_FAILED", errorPtr);
        }
    } catch (const std::exception& e) {
        NSLog(@"Exception in LoginCommand: %s", e.what());
        
        // Send error notification
        NSDictionary* errorData = @{
            @"message": @"Internal error occurred"
        };
        void* errorPtr = (__bridge void*)errorData;
        facade.sendNotification("LOGIN_FAILED", errorPtr);
    } catch (...) {
        NSLog(@"Unknown exception in LoginCommand");
        
        // Send error notification
        NSDictionary* errorData = @{
            @"message": @"Unknown error occurred"
        };
        void* errorPtr = (__bridge void*)errorData;
        facade.sendNotification("LOGIN_FAILED", errorPtr);
    }
}
*/
