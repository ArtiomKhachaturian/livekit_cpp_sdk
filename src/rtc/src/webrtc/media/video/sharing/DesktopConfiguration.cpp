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
#include "VideoUtils.h"
#include "Utils.h"
#ifdef WEBRTC_MAC
#include "CGScreenCapturer.h"
#include "SCKDesktopCapturer.h"
#endif

namespace LiveKitCpp
{

DesktopConfiguration::DesktopConfiguration(std::weak_ptr<webrtc::TaskQueueBase> releaseQueue)
    : _releaseQueue(releaseQueue)
    , _screensEnumerator(createRawCapturer(false, true, true))
    , _windowsEnumerator(createRawCapturer(true, true, true))
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
                                                                      bool previewMode,
                                                                      VideoFrameBufferPool framesPool,
                                                                      bool selectSource)
{
    std::unique_ptr<DesktopCapturer> capturer;
    if (deviceIsScreen(guid)) {
        capturer = createRawCapturer(false, previewMode, false, std::move(framesPool));
    }
    else if (deviceIsWindow(guid)) {
        capturer = createRawCapturer(true, previewMode, false, std::move(framesPool));
    }
    if (capturer && (!selectSource || capturer->selectSource(guid))) {
        return capturer;
    }
    return {};
}

int32_t DesktopConfiguration::boundFramerate(int32_t fps)
{
    if (fps <= 0) {
        fps = webrtc::videocapturemodule::kDefaultFrameRate;
    }
    else {
        fps = bound<int32_t>(1, fps, webrtc::videocapturemodule::kMaxFrameRate); // max 60 Hz
    }
    return fps;
}

std::string_view DesktopConfiguration::logCategory()
{
    static const std::string_view category("desktop_sharing");
    return category;
}

DesktopCapturer* DesktopConfiguration::enumerator(bool windows) const
{
    return windows ? _windowsEnumerator.get() : _screensEnumerator.get();
}

webrtc::DesktopCaptureOptions DesktopConfiguration::makeOptions(bool embeddedCursor)
{
    auto options = webrtc::DesktopCaptureOptions::CreateDefault();
    // Leave desktop effects enabled during WebRTC captures.
    options.set_prefer_cursor_embedded(embeddedCursor);
    // only pure GDI capturers works stable on all devices (enumeration based on device indices)
#ifdef WEBRTC_WIN
    options.set_allow_wgc_screen_capturer(true);
    options.set_allow_wgc_window_capturer(true);
    // force to OFF DirectX capturers
    options.set_allow_directx_capturer(false);
#elif defined(WEBRTC_MAC)
    options.set_allow_iosurface(true);
    options.set_allow_sck_capturer(true);
#endif
    return options;
}

std::unique_ptr<DesktopCapturer> DesktopConfiguration::createRawCapturer(bool window,
                                                                         bool previewMode,
                                                                         bool enumerationOnly,
                                                                         VideoFrameBufferPool framesPool)
{
    std::unique_ptr<DesktopCapturer> impl;
#ifdef WEBRTC_MAC
    if (SCKDesktopCapturer::available()) {
        return std::make_unique<SCKDesktopCapturer>(window, previewMode,
                                                    makeOptions(!previewMode),
                                                    std::move(framesPool));
    }
    if (!window) {
        return std::make_unique<CGScreenCapturer>(previewMode,
                                                  makeOptions(!previewMode),
                                                  commonSharedQueue(enumerationOnly),
                                                  std::move(framesPool));
    }
#endif
    if (!impl) {
        auto options = makeOptions(!previewMode);
#ifdef WEBRTC_WIN
        if (previewMode) {
            options.set_allow_wgc_screen_capturer(false);
            options.set_allow_wgc_window_capturer(false);
        }
#endif
        impl = std::make_unique<DesktopWebRTCCapturer>(window, previewMode,
                                                       std::move(options),
                                                       commonSharedQueue(enumerationOnly),
                                                       std::move(framesPool));
    }
    return impl;
}

std::shared_ptr<webrtc::TaskQueueBase> DesktopConfiguration::commonSharedQueue(bool enumerationOnly)
{
    std::shared_ptr<webrtc::TaskQueueBase> timerQueue;
    if (!enumerationOnly) {
        LOCK_WRITE_SAFE_OBJ(_timerQueue);
        timerQueue = _timerQueue->lock();
        if (!timerQueue) {
            timerQueue = createTaskQueueS("common_sharing_queue",
                                          webrtc::TaskQueueFactory::Priority::HIGH,
                                          _releaseQueue);
            _timerQueue = timerQueue;
        }
    }
    return timerQueue;
}

} // namespace LiveKitCpp
