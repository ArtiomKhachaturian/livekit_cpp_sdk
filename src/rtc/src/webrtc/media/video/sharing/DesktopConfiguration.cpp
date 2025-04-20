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
#include "DesktopConfiguration.h"
#include "DesktopWebRTCCapturer.h"
#include "Utils.h"
#ifdef WEBRTC_MAC
#include "CGScreenCapturer.h"
#include "ScreenCaptureKitCapturer.h"
#endif

namespace LiveKitCpp
{

DesktopConfiguration::DesktopConfiguration()
    : _screensEnumerator(createRawCapturer(false, false))
    , _windowsEnumerator(createRawCapturer(true, true))
{
}

DesktopConfiguration::~DesktopConfiguration()
{
}

webrtc::DesktopSize DesktopConfiguration::screenResolution(const MediaDeviceInfo& dev) const
{
    if (_screensEnumerator) {
        return _screensEnumerator->screenResolution(dev._guid);
    }
    return {};
}

std::vector<MediaDeviceInfo> DesktopConfiguration::enumerate(bool windows) const
{
    std::vector<MediaDeviceInfo> devices;
    if (const auto e = enumerator(windows)) {
        std::vector<std::string> sources;
        if (e->enumerateSources(sources)) {
            devices.reserve(sources.size());
            for (auto& source : sources) {
                MediaDeviceInfo info;
                info._name = e->title(source).value_or(std::string{});
                info._guid = std::move(source);
                devices.push_back(std::move(info));
            }
        }
    }
    return devices;
}

std::vector<MediaDeviceInfo> DesktopConfiguration::enumerateScreens() const
{
    return enumerate(false);
}

std::vector<MediaDeviceInfo> DesktopConfiguration::enumerateWindows() const
{
    return enumerate(true);
}

bool DesktopConfiguration::deviceIsScreen(const std::string& guid) const
{
    return _screensEnumerator && _screensEnumerator->screenIdFromString(guid).has_value();
}

bool DesktopConfiguration::deviceIsScreen(const MediaDeviceInfo& info) const
{
    return deviceIsScreen(info._guid);
}

bool DesktopConfiguration::deviceIsWindow(const std::string& guid) const
{
    return _windowsEnumerator && _windowsEnumerator->windowIdFromString(guid).has_value();
}

bool DesktopConfiguration::deviceIsWindow(const MediaDeviceInfo& info) const
{
    return deviceIsWindow(info._guid);
}

bool DesktopConfiguration::deviceIsValid(const std::string& guid) const
{
    return deviceIsScreen(guid) || deviceIsWindow(guid);
}

bool DesktopConfiguration::deviceIsValid(const MediaDeviceInfo& info) const
{
    return deviceIsScreen(info) || deviceIsWindow(info);
}

bool DesktopConfiguration::hasTheSameType(const std::string& lGuid, const std::string& rGuid) const
{
    return (deviceIsScreen(lGuid) == deviceIsScreen(rGuid)) || (deviceIsWindow(lGuid) == deviceIsWindow(rGuid));
}

bool DesktopConfiguration::hasTheSameType(const MediaDeviceInfo& l, const MediaDeviceInfo& r) const
{
    return hasTheSameType(l._guid, r._guid);
}

std::unique_ptr<DesktopCapturer> DesktopConfiguration::createCapturer(const std::string& guid,
                                                                      bool selectSource,
                                                                      bool lightweightOptions)
{
    std::unique_ptr<DesktopCapturer> capturer;
    if (deviceIsScreen(guid)) {
        capturer = createRawCapturer(false, lightweightOptions);
    }
    else if (deviceIsWindow(guid)) {
        capturer = createRawCapturer(true, lightweightOptions);
    }
    if (capturer && (!selectSource || capturer->selectSource(guid))) {
        return capturer;
    }
    return {};
}

DesktopCapturer* DesktopConfiguration::enumerator(bool windows) const
{
    return windows ? _windowsEnumerator.get() : _screensEnumerator.get();
}

webrtc::DesktopCaptureOptions DesktopConfiguration::makeOptions(bool lightweightMode)
{
    auto options = webrtc::DesktopCaptureOptions::CreateDefault();
    // Leave desktop effects enabled during WebRTC captures.
    options.set_disable_effects(false);
    options.set_detect_updated_region(false);
    // only pure GDI capturers works stable on all devices (enumeration based on device indices)
#ifdef WEBRTC_WIN
    options.set_allow_wgc_capturer(false);
    // force to OFF DirectX capturers
    options.set_allow_directx_capturer(false);
    options.set_allow_use_magnification_api(false);
#elif defined(WEBRTC_MAC)
    options.set_allow_iosurface(true);
    options.set_allow_sck_capturer(true);
    if (lightweightMode) {
        options.set_full_screen_window_detector(nullptr);
        options.set_configuration_monitor(nullptr);
    }
#endif
    return options;
}

std::unique_ptr<DesktopCapturer> DesktopConfiguration::createRawCapturer(bool window,
                                                                         bool lightweightOptions)
{
    std::unique_ptr<DesktopCapturer> impl;
#ifdef WEBRTC_MAC
    if (ScreenCaptureKitCapturer::available()) {
        return std::make_unique<ScreenCaptureKitCapturer>(window, makeOptions(lightweightOptions));
    }
    if (!window) {
        return std::make_unique<CGScreenCapturer>(makeOptions(lightweightOptions), commonSharedQueue());
    }
#endif
    if (!impl) {
        impl = std::make_unique<DesktopWebRTCCapturer>(window, makeOptions(lightweightOptions), commonSharedQueue());
    }
    return impl;
}

std::shared_ptr<webrtc::TaskQueueBase> DesktopConfiguration::commonSharedQueue()
{
    LOCK_WRITE_SAFE_OBJ(_timerQueue);
    if (!_timerQueue.constRef()) {
        _timerQueue = createTaskQueueS("common_sharing_queue", webrtc::TaskQueueFactory::Priority::HIGH);
    }
    return _timerQueue.constRef();
}

} // namespace LiveKitCpp
