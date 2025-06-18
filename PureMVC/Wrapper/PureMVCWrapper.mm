//
//  PureMVCWrapper.m
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#import "PureMVCWrapper.h"
#include "PureMVC/Patterns/Facade/Facade.hpp"
#include "PureMVC/Core/View.hpp"
#include "PureMVC/Interfaces/IObserver.hpp"
#include "PureMVC/PureMVC.hpp"
#include "PureMVCConstants.h"
#include "UserProxy.h"
#include "LoginCommand.h"
#include <string>
#include <vector>
#import "PureMVCConstantsWrapper.h"

using namespace PureMVC;
using namespace PureMVC::Core;
using namespace PureMVC::Patterns;
using namespace PureMVC::Interfaces;
using namespace PureMVCConstants;

// Custom Observer implementation
class WrapperObserver : public IObserver {
private:
    __weak PureMVCWrapper* wrapper;
    void* context;
    
public:
    WrapperObserver(__weak PureMVCWrapper* objcWrapper)
        : wrapper(objcWrapper), context((__bridge void*)objcWrapper) {
        NSLog(@"WrapperObserver created");
    }
    
    virtual void notifyObserver(INotification const& notification) override {
        NSLog(@"WrapperObserver::notifyObserver called for: %s", notification.getName().c_str());
        
        if (wrapper && wrapper.delegate) {
            NSString* noteName = [NSString stringWithUTF8String:notification.getName().c_str()];
            
            // Call delegate on main thread
            dispatch_async(dispatch_get_main_queue(), ^{
                [wrapper.delegate onNotificationReceived:noteName withData:nil];
            });
        }
    }
    
    virtual bool compareNotifyContext(void const* object) const override {
        return context == object;
    }
    
    virtual ~WrapperObserver() {
        NSLog(@"WrapperObserver destroyed");
    }
};

// Commands implementations...
class LogoutCommand : public SimpleCommand {
public:
    virtual void execute(INotification const& notification) override {
        NSLog(@"LogoutCommand::execute called");
        
        IFacade& facade = getFacade();
        try {
            IProxy& baseProxy = facade.retrieveProxy(UserProxy::NAME);
            UserProxy* userProxy = dynamic_cast<UserProxy*>(&baseProxy);
            
            if (userProxy) {
                userProxy->logout();
                facade.sendNotification(LOGOUT_SUCCESS);
                NSLog(@"User logged out");
            }
        } catch (...) {
            NSLog(@"Failed to retrieve UserProxy");
        }
    }
};

class DataRefreshCommand : public SimpleCommand {
public:
    virtual void execute(INotification const& notification) override {
        NSLog(@"DataRefreshCommand::execute called");
        
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
                      dispatch_get_main_queue(), ^{
            NSLog(@"Data refreshed!");
        });
    }
};

// Static storage
static LoginCommand* loginCmd = nullptr;
static LogoutCommand* logoutCmd = nullptr;
static DataRefreshCommand* refreshCmd = nullptr;
static std::vector<IObserver*> observers;

@implementation PureMVCWrapper

+ (instancetype)sharedInstance {
    static PureMVCWrapper *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[PureMVCWrapper alloc] init];
    });
    return instance;
}

- (void)initializeFacade {
    NSLog(@"Initializing PureMVC Facade...");
    
    // Get facade instance
    IFacade& facade = Facade::getInstance(FACADE_NAME);
    
    // Get View instance for observer registration
    IView& view = View::getInstance(FACADE_NAME);
    
    // Register Proxy
    UserProxy* userProxy = new UserProxy();
    facade.registerProxy(userProxy);
    
    // Create and register Observers
    WrapperObserver* loginSuccessObs = new WrapperObserver(self);
    WrapperObserver* loginFailedObs = new WrapperObserver(self);
    WrapperObserver* logoutSuccessObs = new WrapperObserver(self);
    
    // Register observers with View
    view.registerObserver(LOGIN_SUCCESS, loginSuccessObs);
    view.registerObserver(LOGIN_FAILED, loginFailedObs);
    view.registerObserver(LOGOUT_SUCCESS, logoutSuccessObs);
    
    // Store observers
    observers.push_back(loginSuccessObs);
    observers.push_back(loginFailedObs);
    observers.push_back(logoutSuccessObs);
    
    // Create and register commands
    loginCmd = new LoginCommand();
    logoutCmd = new LogoutCommand();
    refreshCmd = new DataRefreshCommand();
    
    facade.registerCommand(LOGIN_REQUEST, loginCmd);
    facade.registerCommand(LOGOUT_REQUEST, logoutCmd);
    facade.registerCommand(DATA_REFRESH, refreshCmd);
    
    NSLog(@"PureMVC Facade initialized successfully");
}

- (void)sendNotification:(NSString *)notificationName {
    [self sendNotification:notificationName withBody:nil];
}

- (void)sendNotification:(NSString *)notificationName withBody:(id)body {
    IFacade& facade = Facade::getInstance(FACADE_NAME);
    
    std::string noteName = [notificationName UTF8String];
    
    if (body) {
        void* bodyPtr = (__bridge void*)body;
        facade.sendNotification(noteName, bodyPtr);
    } else {
        facade.sendNotification(noteName);
    }
}

- (void)onLoginButtonPressed:(NSString *)username password:(NSString *)password {
    NSLog(@"Login button pressed - Username: %@", username);
    
    NSDictionary *loginData = @{
        @"username": username,
        @"password": password
    };
    
    [self sendNotification:PureMVCNotifications.loginRequest withBody:loginData];
}

- (void)onLogoutButtonPressed {
    NSLog(@"Logout button pressed");
    [self sendNotification:PureMVCNotifications.logoutRequest];
}

- (void)onDataRefreshRequested {
    NSLog(@"Data refresh requested");
    [self sendNotification:PureMVCNotifications.dataRefresh];
}

- (void)dealloc {
    // Clean up observers
    for (auto obs : observers) {
        delete obs;
    }
    observers.clear();
}

@end
