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
#include "DesktopCapturerUtils.h"
#include "DesktopCapturer.h"
#include "CGWDescription.h"
#include "Utils.h"
#include <modules/desktop_capture/mac/desktop_configuration_monitor.h>
#include <modules/desktop_capture/mac/window_list_utils.h>
#include <optional>
#import <AppKit/NSScreen.h>
#import <IOKit/graphics/IOGraphicsLib.h>

static_assert(std::is_same_v<CGDirectDisplayID, uint32_t>);
static_assert(std::is_same_v<CGWindowID, uint32_t>);

namespace
{

using namespace LiveKitCpp;

template <typename MacDisplay>
webrtc::DesktopSize resolution(const MacDisplay& display);

std::string ioGraphicLibScreenName(CGDirectDisplayID guid);

io_service_t displayIOServicePort(CGDirectDisplayID guid);


template <typename T>
inline std::optional<T> number(CFNumberRef ref, CFNumberType type) {
    if (ref) {
        T value = {};
        if (CFNumberGetValue(ref, type, &value)) {
            return value;
        }
    }
    return std::nullopt;
}

inline bool isHiddenWindow(webrtc::WindowId wId)
{
    if (const auto desc = CGWDescription::create(static_cast<CGWindowID>(wId))) {
        if (!desc->isWindowServerWindow() && !startWith(stringFromCFString(desc->ownerName()), "ControlUp")) {
            if (const auto sharingState = number<int>(desc->sharingState(), kCFNumberIntType)) {
                switch (sharingState.value()) {
                    case kCGWindowSharingReadOnly:
                    case kCGWindowSharingReadWrite:
                        return false;
                    default:
                        break;
                }
            }
        }
    }
    return true;
}

}

namespace LiveKitCpp
{

bool enumerateWindows(const webrtc::DesktopCaptureOptions&, std::vector<std::string>& out)
{
    webrtc::DesktopCapturer::SourceList list;
    if (webrtc::GetWindowList(&list, true, true)) {
        out.clear();
        out.reserve(list.size());
        for (const auto& source : list) {
            const auto wId = static_cast<webrtc::WindowId>(source.id);
            if (!isHiddenWindow(wId)) {
                out.push_back(windowIdToString(wId));
            }
        }
        return true;
    }
    return false;
}

bool screenExists(webrtc::ScreenId sId)
{
    if (webrtc::kInvalidScreenId != sId) {
        if (webrtc::kFullDesktopScreenId == sId) {
            const auto guid = static_cast<CGDirectDisplayID>(sId);
            if (@available(macOS 10.15, *)) {
                @autoreleasepool {
                    return nil != findScreen(guid);
                }
            }
            // fallback to earlier OS versions
            return 0 != displayIOServicePort(guid);
        }
        return true; // desktop is always exists
    }
    return false;
}

bool windowExists(webrtc::WindowId wId)
{
    if (!isHiddenWindow(wId)) {
        const auto id = static_cast<CGWindowID>(wId);
        return nullptr != CGWDescription::create(id).get();
    }
    return false;
}

std::string screenTitle(webrtc::ScreenId sId)
{
    if (webrtc::kInvalidScreenId != sId) {
        if (webrtc::kFullDesktopScreenId == sId) {
            return "Full desktop";
        }
        const auto guid = static_cast<CGDirectDisplayID>(sId);
        if (@available(macOS 10.15, *)) {
            @autoreleasepool {
                NSScreen* screen = findScreen(guid);
                if (screen) {
                    return fromNSString(screen.localizedName);
                }
            }
        }
        return ioGraphicLibScreenName(guid); // fallback to earlier OS versions
    }
    return std::string();
}

std::optional<std::string> windowTitle(webrtc::WindowId wId)
{
    if (webrtc::kNullWindowId != wId) {
        if (const auto winDesc = CGWDescription::create(static_cast<CGWindowID>(wId))) {
            auto titleRef = winDesc->name();
            if (!titleRef || 0L == CFStringGetLength(titleRef)) {
                titleRef = winDesc->ownerName();
            }
            if (titleRef) {
                return stringFromCFString(titleRef);
            }
        }
    }
    return std::nullopt;
}

webrtc::DesktopSize screenResolution(const webrtc::DesktopCaptureOptions& options, webrtc::ScreenId sId)
{
    if (const auto& monitor = options.configuration_monitor()) {
        auto desktopConfig = monitor->desktop_configuration();
        if (webrtc::kFullDesktopScreenId != static_cast<webrtc::ScreenId>(sId)) {
            const auto id = static_cast<CGDirectDisplayID>(sId);
            if (const auto monitor = desktopConfig.FindDisplayConfigurationById(id)) {
                return resolution(*monitor);
            }
        } else {
            return resolution(desktopConfig);
        }
    }
    return {};
}

bool enumerateScreens(const webrtc::DesktopCaptureOptions& options, std::vector<std::string>& out)
{
    if (const auto& monitor = options.configuration_monitor()) {
        const auto config = monitor->desktop_configuration();
        out.clear();
        out.reserve(config.displays.size());
        for (auto it = config.displays.begin(); it != config.displays.end(); ++it) {
            out.push_back(screenIdToString(it->id));
        }
        return true;
    }
    return false;
}

NSScreen* findScreen(CGDirectDisplayID guid)
{
    NSScreen* result = nil;
    @autoreleasepool {
        auto screens = [NSScreen screens];
        const auto count = screens ? [screens count] : 0U;
        if (count > 0U) {
            if (1U == count) {
                result = [screens firstObject];
            }
            else {
                for (NSScreen* screen in screens) {
                    auto description = [screen deviceDescription];
                    if (guid == [description[@"NSScreenNumber"] unsignedIntValue]) {
                        result = screen;
                    }
                }
            }
        }
    }
    return result;
}

} // namespace LiveKitCpp

namespace
{

template <typename MacDisplay>
webrtc::DesktopSize resolution(const MacDisplay& display)
{
    const float dpiToPixelScale = std::max(1.f, display.dip_to_pixel_scale);
    const auto s = display.bounds.size();
    return webrtc::DesktopSize(static_cast<int32_t>(std::round(dpiToPixelScale * s.width())),
                               static_cast<int32_t>(std::round(dpiToPixelScale * s.height())));
}

std::string ioGraphicLibScreenName(CGDirectDisplayID guid)
{
    if (const auto serv = displayIOServicePort(guid)) {
        const CFDictionaryRefAutoRelease info(IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName));
        if (const auto names = reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(info, CFSTR(kDisplayProductName)))) {
            CFStringRef value;
            if (CFDictionaryGetValueIfPresent(names, CFSTR("en_US"), (const void**)&value)) {
                return stringFromCFString(value);
            }
        }
    }
    return {};
}

io_service_t displayIOServicePort(CGDirectDisplayID guid)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    // the port is owned by the graphics system and should not be destroyed
    return CGDisplayIOServicePort(guid);
#pragma clang diagnostic pop
}

}
