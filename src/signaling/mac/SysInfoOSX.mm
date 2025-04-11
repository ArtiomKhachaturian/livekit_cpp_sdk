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
#include "SysInfo.h"
#include "livekit/signaling/NetworkType.h"
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <CoreWLAN/CoreWLAN.h>

namespace {

inline std::string fromNSString(NSString* nsString)
{
    if (nsString) {
        @autoreleasepool {
            NSData* charData = [nsString dataUsingEncoding:NSUTF8StringEncoding];
            if (charData) {
                return std::string(reinterpret_cast<const char *>(charData.bytes),
                                   charData.length);
            }
        }
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
            model = fromNSString(modelId);
        }
        IOObjectRelease(service);
    }
    return model;
}

NetworkType activeNetworkType()
{
    NetworkType type = NetworkType::Unknown;
    @autoreleasepool {
        if (const auto storeRef = SCDynamicStoreCreate(nullptr, CFSTR("GetActiveNetworkType"),
                                                       nullptr, nullptr)) {
            if (const auto global = SCDynamicStoreCopyValue(storeRef, CFSTR("State:/Network/Global/IPv4"))) {
                
                NSDictionary* dict = (__bridge NSDictionary *)global;
                NSString *primaryInterface = dict[@"PrimaryInterface"];
                // check network type
                auto wifiInterface = [CWWiFiClient sharedWiFiClient].interface;
                if ([primaryInterface isEqualToString:wifiInterface.interfaceName]) {
                    type = NetworkType::WiFi;
                } else if ([primaryInterface hasPrefix:@"en"]) {
                    type = NetworkType::Wired;
                } else if ([primaryInterface hasPrefix:@"pdp_ip"]) {
                    type = NetworkType::Cellular;
                } else if ([primaryInterface hasPrefix:@"utun"]) {
                    type = NetworkType::Vpn;
                }
                CFRelease(global);
            }
            else {
                type = NetworkType::NoNetwork;
            }
            CFRelease(storeRef);
        }
    }
    return type;
}

} // namespace LiveKitCpp
