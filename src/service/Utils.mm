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
#include "Utils.h"
#import <Foundation/Foundation.h>

namespace {

inline std::string NSStringToStdString(NSString* nsString) {
    if (nsString) {
        return std::string(nsString.UTF8String);
    }
    return {};
}

}

namespace LiveKitCpp
{

std::string operatingSystemVersion()
{
    @autoreleasepool {
        NSProcessInfo *processInfo = [NSProcessInfo processInfo];
        NSOperatingSystemVersion osv = processInfo.operatingSystemVersion;
        return std::to_string(osv.majorVersion) + "." +
               std::to_string(osv.minorVersion) + "." +
               std::to_string(osv.patchVersion);
    }
}

std::string operatingSystemName()
{
    return "macos";
}

std::string modelIdentifier()
{
    std::string model;
    @autoreleasepool {
        const auto service = IOServiceGetMatchingService(kIOMainPortDefault,
                                                         IOServiceMatching("IOPlatformExpertDevice"));
        const auto ref = IORegistryEntryCreateCFProperty(service, CFSTR("model"),
                                                         kCFAllocatorDefault, 0);
        if (ref) {
            NSData *modelData = (__bridge_transfer NSData *)ref;
            NSString* modelId = [[NSString alloc] initWithData:modelData encoding:NSUTF8StringEncoding];
            modelId = [modelId stringByTrimmingCharactersInSet:[NSCharacterSet controlCharacterSet]];
            model = NSStringToStdString(modelId);
        }
        IOObjectRelease(service);
    }
    return model;
}

} // namespace LiveKitCpp
