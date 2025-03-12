// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifdef __APPLE__
#include "AppEnvironment.h"
#include "Utils.h"
#import <Foundation/Foundation.h>

namespace {

NSDictionary* readApplicationInfoPList();
bool requiredKeysIsPresent(NSDictionary* infoPlist, std::string* additionalErrorInfo = nullptr);
bool hasMainThread();

}

namespace LiveKitCpp
{

unsigned checkAppEnivonment(unsigned expectedStatus, std::string* additionalErrorInfo)
{
    if (additionalErrorInfo) {
        additionalErrorInfo->clear();
    }
    unsigned actualStatus = AESNoProblems;
    if (AESNoProblems == expectedStatus) {
        if (!hasMainThread()) {
            actualStatus |= AESNoGuiThread;
        }
        @autoreleasepool {
            if (NSDictionary* plist = readApplicationInfoPList()) {
                if (!requiredKeysIsPresent(plist, additionalErrorInfo)) {
                    actualStatus |= AESIncompleteInfoPlist;
                }
            }
            else {
                actualStatus |= AESNoInfoPlist;
            }
        }
    }
    else {
        if (testFlag<AESNoGuiThread>(expectedStatus) && !hasMainThread()) {
            actualStatus |= AESNoGuiThread;
        }
        if (testFlag<AESNoInfoPlist>(expectedStatus) ||
            testFlag<AESIncompleteInfoPlist>(expectedStatus)) {
            @autoreleasepool {
                if (NSDictionary* plist = readApplicationInfoPList()) {
                    if (testFlag<AESIncompleteInfoPlist>(expectedStatus) &&
                        !requiredKeysIsPresent(plist, additionalErrorInfo)) {
                        actualStatus |= AESIncompleteInfoPlist;
                    }
                }
                else {
                    actualStatus |= AESNoInfoPlist;
                }
            }
        }
    }
    return actualStatus;
}

std::string formatStatusMessage(unsigned status, std::string additionalInfo)
{
    if (AESNoProblems != status) {
        std::string message;
        if (testFlag<AESNoGuiThread>(status)) {
            message += std::string("Main thread is not available");
        }
    }
    return std::string();
}

    
} // namespace LiveKitCpp

namespace {

NSString* const g_keys [] = {
    @"NSCameraUsageDescription",
    @"NSMicrophoneUsageDescription",
    @"NSHighResolutionCapable",
    @"NSPrincipalClass"
};

NSDictionary* readApplicationInfoPList()
{
    @autoreleasepool {
        NSString* path = [[NSBundle mainBundle] bundlePath];
        if (NSDictionary* d1 = [NSDictionary dictionaryWithContentsOfFile: [path stringByAppendingPathComponent:@"Info.plist"]]) {
            return d1;
        }
        if (NSDictionary* d2 = [NSDictionary dictionaryWithContentsOfFile: [path stringByAppendingPathComponent:@"Contents/Info.plist"]]) {
            return d2;
        }
    }
    return nil;
}

bool requiredKeysIsPresent(NSDictionary* infoPlist, std::string* additionalErrorInfo)
{
    if (additionalErrorInfo) {
        additionalErrorInfo->clear();
    }
    if (infoPlist) {
        @autoreleasepool {
            for (NSString* key : infoPlist) {
                if (nil == [infoPlist valueForKey:key]) {
                    if (additionalErrorInfo) {
                        *additionalErrorInfo = "Application bundle info dictionary doesn't contain '" +
                            LiveKitCpp::fromNSString(key) + "' key, some features of LiveKit SDK may be works incorrectly";
                    }
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool hasMainThread()
{
    @autoreleasepool {
        return nil != [NSThread mainThread];
    }
}

}
#endif
