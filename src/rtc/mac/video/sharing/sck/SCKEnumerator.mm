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
#include "SCKEnumerator.h"
#include "ConditionWaiter.h"
#include "DesktopCapturerUtils.h"
#import <ScreenCaptureKit/ScreenCaptureKit.h>

namespace LiveKitCpp
{

SCKEnumerator::SCKEnumerator()
{
}

SCKEnumerator::~SCKEnumerator()
{
    _content = nil;
}

NSError* SCKEnumerator::updateContent()
{
    using Waiter = ConditionWaiter<SCShareableContent*, NSError*>;
    const auto waiter = std::make_shared<Waiter>();
    [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent * _Nullable content,
                                                                   NSError * _Nullable error) {
        if (error) {
            waiter->setValue(error);
        }
        else {
            waiter->setValue(content);
        }
    }];
    waiter->wait();
    _content = waiter->getRef<SCShareableContent*>();
    return waiter->getRef<NSError*>();
}

SCWindow* SCKEnumerator::toWindow(const std::string& source) const
{
    if (const auto wId = windowIdFromString(source)) {
        return toWindow(wId.value());
    }
    return nil;
}

SCWindow* SCKEnumerator::toWindow(webrtc::WindowId wId) const
{
    if (webrtc::kNullWindowId != wId && _content) {
        @autoreleasepool {
            const auto windows = _content.windows;
            for (SCWindow* window in windows) {
                if (window.windowID == wId) {
                    return window;
                }
            }
        }
    }
    return nil;
}

SCDisplay* SCKEnumerator::toScreen(const std::string& source) const
{
    if (const auto sId = screenIdFromString(source)) {
        return toScreen(sId.value());
    }
    return nil;
}

SCDisplay* SCKEnumerator::toScreen(webrtc::ScreenId sId) const
{
    if (webrtc::kInvalidScreenId != sId && webrtc::kInvalidDisplayId != sId
        && _content) {
        @autoreleasepool {
            const auto displays = _content.displays;
            for (SCDisplay* display in displays) {
                if (display.displayID == sId) {
                    return display;
                }
            }
        }
    }
    return nil;
}

std::string SCKEnumerator::windowToString(SCWindow* window)
{
    if (window) {
        return windowIdToString(window.windowID);
    }
    return {};
}

std::string SCKEnumerator::displayToString(SCDisplay* display)
{
    if (display) {
        return screenIdToString(display.displayID);
    }
    return {};
}

} // namespace LiveKitCpp
