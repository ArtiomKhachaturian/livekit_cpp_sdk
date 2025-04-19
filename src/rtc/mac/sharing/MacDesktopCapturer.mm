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
#include "MacDesktopCapturer.h"
#include "CFAutoRelease.h"
#include "CGWDescription.h"
#include "CGDesktopCapturer.h"
#include "VideoUtils.h"
#include "VTSupportedPixelFormats.h"
#include "Utils.h"
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/mac/desktop_configuration_monitor.h>
#include <modules/desktop_capture/mac/window_list_utils.h>
#include <modules/desktop_capture/mac/desktop_frame_cgimage.h>
#include <CoreGraphics/CGDisplayConfiguration.h> // for CGWindowID
#import <AppKit/NSScreen.h>
#import <IOKit/graphics/IOGraphicsLib.h>

static_assert(std::is_same_v<CGDirectDisplayID, uint32_t>);
static_assert(std::is_same_v<CGWindowID, uint32_t>);

namespace {

template <typename MacDisplay>
webrtc::DesktopSize resolution(const MacDisplay& display);

std::string ioGraphicLibScreenName(CGDirectDisplayID guid);

io_service_t displayIOServicePort(CGDirectDisplayID guid);

}

namespace LiveKitCpp
{

MacDesktopCapturer::MacDesktopCapturer(bool window, const webrtc::scoped_refptr<webrtc::DesktopConfigurationMonitor>& monitor)
    : DesktopCapturer(window)
    , _monitor(monitor)
{
}

MacDesktopCapturer::~MacDesktopCapturer()
{
}

bool MacDesktopCapturer::isWindowServerWindow(webrtc::WindowId wId)
{
    return CGWDescription::isWindowServerWindow(static_cast<CGWindowID>(wId));
}

bool MacDesktopCapturer::screenExists(webrtc::ScreenId sId)
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

bool MacDesktopCapturer::windowExists(webrtc::WindowId wId)
{
    if (!isWindowServerWindow(wId)) {
        const auto id = static_cast<CGWindowID>(wId);
        return nullptr != CGWDescription::create(id).get();
    }
    return false;
}

std::string MacDesktopCapturer::screenTitle(webrtc::ScreenId sId)
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

std::optional<std::string> MacDesktopCapturer::windowTitle(webrtc::WindowId wId)
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

webrtc::DesktopSize MacDesktopCapturer::screenResolution(const webrtc::scoped_refptr<webrtc::DesktopConfigurationMonitor>& monitor,
                                                         webrtc::ScreenId sId)
{
    if (monitor) {
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

std::unique_ptr<DesktopCapturer> MacDesktopCapturer::create(bool window,
                                                            const webrtc::DesktopCaptureOptions& options,
                                                            const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue)
{
    if (@available(macOS 12.3, *)) {
        // ScreenCaptureKitCapturer
    }
    return std::make_unique<CGDesktopCapturer>(timerQueue, window, options);
}

std::optional<std::string> MacDesktopCapturer::title(const std::string& source) const
{
    if (window()) {
        if (const auto wId = windowIdFromString(source)) {
            return windowTitle(wId.value());
        }
    }
    else if (const auto sId = screenIdFromString(source)) {
        auto title = screenTitle(sId.value());
        if (!title.empty()) {
            return std::make_optional<std::string>(std::move(title));
        }
    }
    return std::nullopt;
}

webrtc::DesktopSize MacDesktopCapturer::screenResolution(const std::string& source) const
{
    if (!window()) {
        if (const auto sId = screenIdFromString(source)) {
            return screenResolution(_monitor, sId.value());
        }
    }
    return {};
}

bool MacDesktopCapturer::enumerateSources(std::vector<std::string>& sources) const
{
    if (window()) {
        webrtc::DesktopCapturer::SourceList list;
        if (webrtc::GetWindowList(&list, true, true)) {
            sources.clear();
            sources.reserve(list.size());
            for (const auto& source : list) {
                const auto wId = static_cast<webrtc::WindowId>(source.id);
                if (!isWindowServerWindow(wId)) {
                    sources.push_back(windowIdToString(wId));
                }
            }
            return true;
        }
    }
    else if (_monitor) {
        const auto config = _monitor->desktop_configuration();
        sources.clear();
        sources.reserve(config.displays.size());
        for (auto it = config.displays.begin(); it != config.displays.end(); ++it) {
            sources.push_back(screenIdToString(it->id));
        }
        return true;
    }
    return false;
}


void MacDesktopCapturer::setSharedMemoryFactory(std::unique_ptr<webrtc::SharedMemoryFactory> smf)
{
    _smf.reset(smf.release());
}

std::unique_ptr<webrtc::DesktopFrame> MacDesktopCapturer::captureDisplay(webrtc::ScreenId sId) const
{
    const auto id = static_cast<CGDirectDisplayID>(sId);
    return webrtc::DesktopFrameCGImage::CreateForDisplay(id);
}

std::unique_ptr<webrtc::DesktopFrame> MacDesktopCapturer::captureWindow(webrtc::WindowId wId) const
{
    const auto id = static_cast<CGWindowID>(wId);
    return webrtc::DesktopFrameCGImage::CreateForWindow(id);
}

OSType MacDesktopCapturer::recommendedVideoFormat()
{
    return pixelFormatNV12Video();
}

dispatch_queue_t MacDesktopCapturer::currentQueue()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    return dispatch_get_current_queue();
#pragma GCC diagnostic pop
}

} // namespace LiveKitCpp

namespace
{

using namespace LiveKitCpp;

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
