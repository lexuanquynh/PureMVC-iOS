//
//  StringHelper.h
//  PureMVC
//
//  Created by Prank on 18/6/25.
//

#ifndef StringHelper_h
#define StringHelper_h
#import <Foundation/Foundation.h>
#include <string>

// Helper functions for Objective-C++ files only

inline NSString* safeStringFromStdString(const std::string& str, NSString* defaultValue = @"") {
    if (str.empty()) {
        return defaultValue;
    }
    
    @try {
        NSString* result = [NSString stringWithUTF8String:str.c_str()];
        return result ?: defaultValue;
    } @catch (NSException *exception) {
        NSLog(@"Exception converting string: %@", exception);
        return defaultValue;
    }
}

inline std::string safeStdStringFromNSString(NSString* str, const std::string& defaultValue = "") {
    if (!str || str.length == 0) {
        return defaultValue;
    }
    
    const char* cStr = [str UTF8String];
    return cStr ? std::string(cStr) : defaultValue;
}

#endif /* StringHelper_h */
