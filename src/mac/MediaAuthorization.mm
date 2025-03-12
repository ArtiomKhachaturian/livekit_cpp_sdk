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
// limitations under the License.#include "MediaAuthorization.h"
#include "MediaAuthorizationCallback.h"
#include "MediaAuthorization.h"
#include "CFAutoRelease.h"
//#include "Sharing/MacOS/CGWindowDescription.h"
//#include "Platform.h"
//#include "VTSupportedPixelFormats.h"
#include <modules/desktop_capture/mac/window_list_utils.h>
#include <optional>
#import <AVFoundation/AVCaptureDevice.h>
#import <ApplicationServices/ApplicationServices.h>
#import <CoreGraphics/CGDisplayStream.h>

#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace {

using namespace LiveKitCpp;

std::optional<AVAuthorizationStatus> statusForMediaType(MediaAuthorizationKind kind,
                                                        AVMediaType* avMediaType = nullptr);
MediaAuthorizationStatus askScreenRecordingStatus(bool windowCapturing, bool askPermissions);

}

namespace LiveKitCpp
{

void MediaAuthorization::requestFromSystem(MediaAuthorizationKind kind, bool askPermissions,
                                           const std::shared_ptr<MediaAuthorizationCallback>& callback)
{
    if (callback) {
        if (MediaAuthorizationKind::ScreenCapturing == kind || MediaAuthorizationKind::WindowCapturing == kind) {
            const auto windowCapturing = MediaAuthorizationKind::WindowCapturing == kind;
            const auto status = askScreenRecordingStatus(windowCapturing, askPermissions);
            callback->completed(status);
        }
        else {
            AVMediaType type;
            if (const auto avStatus = statusForMediaType(kind, &type)) {
                switch (avStatus.value()) {
                    case AVAuthorizationStatusAuthorized:
                        callback->completed(MediaAuthorizationStatus::Granted);
                        break;
                    case AVAuthorizationStatusDenied: // the user has previously denied access
                        callback->completed(MediaAuthorizationStatus::Denied);
                        break;
                    case AVAuthorizationStatusRestricted: // the user can't grant access due to restrictions
                        callback->completed(MediaAuthorizationStatus::Restricted);
                        break;
                    case AVAuthorizationStatusNotDetermined:
                        if (askPermissions) {
                            @autoreleasepool {
                                // The app hasn't yet asked the user for media access.
                                __block auto blockCallback = callback;
                                [AVCaptureDevice requestAccessForMediaType:type completionHandler:^(BOOL granted) {
                                    const auto completedStatus = granted ? MediaAuthorizationStatus::Granted : MediaAuthorizationStatus::Denied;
                                    blockCallback->completed(completedStatus);
                                }];
                            }
                        }
                        else {
                            callback->completed(MediaAuthorizationStatus::NotDetermined);
                        }
                        break;
                    default:
                        callback->completed(MediaAuthorizationStatus::NotDetermined); // new AVAuthorizationStatus?
                        break;
                }
            }
            else {
                callback->completed(MediaAuthorizationStatus::NotDetermined);
            }
        }
    }
}

} // namespace LiveKitCpp

namespace {

inline bool hasScreenCaptureAccess(bool askPermissions)
{
    bool has = false; // pessimistic
    // usage of CGRequestScreenCaptureAccess/CGPreflightScreenCaptureAccess may leads to crash (DVC-17267):
    // http://codeworkshop.net/objc-diff/sdkdiffs/macos/11.0b3/CoreGraphics.html
    // seems like it working stable since 11.x (BigSur), but according to Apple docs this API available since 10.15
    if (@available(macOS 11.0, *)) {
        if (askPermissions) {
            // requests event listening access if absent, potentially prompting sys msgbox
            has = CGRequestScreenCaptureAccess();
        }
        else { // silent mode
            // https://developer.apple.com/forums/thread/683860
            // checks whether the current process already has screen capture access
            has = CGPreflightScreenCaptureAccess();
        }
    }
    else if (@available(macOS 10.15, *)) {
        if (askPermissions) {
            CGDisplayStreamFrameAvailableHandler fakeHandler = ^(CGDisplayStreamFrameStatus, uint64_t,
                                                                 IOSurfaceRef, CGDisplayStreamUpdateRef) {};
            CFAutoRelease<CGDisplayStreamRef> stream = CGDisplayStreamCreate(CGMainDisplayID(), 100UL, 100UL,
                                                                             kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange,
                                                                             nullptr, fakeHandler);
            if (stream) {
                CGDisplayStreamStop(stream);
                has =  true;
            }
        }
        else { // silent mode
            // TODO: not yet implemented
        }
    }
    else {
        // fallback on earlier versions
        // macos 10.14 and lower do not require screen recording permission
        has = true;
    }
    return has;
}

inline bool isTrustedAccessibilityClient(bool askPermissions)
{
    @autoreleasepool {
        NSDictionary* options = @{(__bridge NSString*)kAXTrustedCheckOptionPrompt: askPermissions ? @YES : @NO};
        if (!AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options)) {
            NSString* exePath = NSProcessInfo.processInfo.arguments[0];
            return kAXErrorSuccess == AXMakeProcessTrusted((__bridge CFStringRef)exePath);
        }
    }
    return true;
}

inline std::optional<AVMediaType> mediaType(MediaAuthorizationKind kind)
{
    switch (kind) {
        case MediaAuthorizationKind::Camera:
            return AVMediaTypeVideo;
        case MediaAuthorizationKind::Microphone:
            return AVMediaTypeAudio;
        default:
            break;
    }
    return std::nullopt;
}

std::optional<AVAuthorizationStatus> statusForMediaType(MediaAuthorizationKind kind, AVMediaType* avMediaType)
{
    if (const auto mt = mediaType(kind)) {
        if (avMediaType) {
            *avMediaType = mt.value();
        }
        return [AVCaptureDevice authorizationStatusForMediaType: mt.value()];
    }
    return std::nullopt;
}

MediaAuthorizationStatus askScreenRecordingStatus(bool windowCapturing, bool askPermissions)
{
    auto status = hasScreenCaptureAccess(askPermissions) ? MediaAuthorizationStatus::Granted : MediaAuthorizationStatus::Denied;
    if (windowCapturing && MediaAuthorizationStatus::Granted == status) {
        if (!isTrustedAccessibilityClient(askPermissions)) {
            status = MediaAuthorizationStatus::Restricted;
        }
    }
    return status;
}

}

#ifdef __clang__
#pragma GCC diagnostic pop
#endif
