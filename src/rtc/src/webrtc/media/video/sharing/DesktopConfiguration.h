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
#pragma once // DesktopConfiguration.h
#include "SafeObjAliases.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <api/scoped_refptr.h>
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/desktop_geometry.h>
#include <optional>
#include <memory>

namespace webrtc {
class TaskQueueBase;
}

namespace LiveKitCpp
{

class DesktopCapturer;

class DesktopConfiguration
{
public:
    DesktopConfiguration();
    ~DesktopConfiguration();
    bool screensEnumerationIsAvailable() const { return nullptr != _screensEnumerator; }
    bool windowsEnumerationIsAvailable() const { return nullptr != _windowsEnumerator; }
    webrtc::DesktopSize screenResolution(const MediaDeviceInfo& dev) const;
    std::vector<MediaDeviceInfo> enumerate(bool windows) const;
    std::vector<MediaDeviceInfo> enumerateScreens() const;
    std::vector<MediaDeviceInfo> enumerateWindows() const;
    bool deviceIsScreen(const std::string& guid) const;
    bool deviceIsScreen(const MediaDeviceInfo& info) const;
    bool deviceIsWindow(const std::string& guid) const;
    bool deviceIsWindow(const MediaDeviceInfo& info) const;
    bool deviceIsValid(const std::string& guid) const;
    bool deviceIsValid(const MediaDeviceInfo& info) const;
    bool hasTheSameType(const std::string& lGuid, const std::string& rGuid) const;
    bool hasTheSameType(const MediaDeviceInfo& l, const MediaDeviceInfo& r) const;
    std::unique_ptr<DesktopCapturer> createCapturer(const std::string& guid,
                                                    bool selectSource = false,
                                                    bool lightweightOptions = false);
    static int32_t maxFramerate() { return 30; }
    static std::string_view logCategory();
private:
    DesktopCapturer* enumerator(bool windows) const;
    static webrtc::DesktopCaptureOptions makeOptions(bool lightweightMode = false);
    std::unique_ptr<DesktopCapturer> createRawCapturer(bool window,
                                                       bool lightweightOptions = false);
    std::shared_ptr<webrtc::TaskQueueBase> commonSharedQueue();
private:
    Bricks::SafeSharedPtr<webrtc::TaskQueueBase> _timerQueue;
    std::unique_ptr<DesktopCapturer> _screensEnumerator;
    std::unique_ptr<DesktopCapturer> _windowsEnumerator;
};
	
} // namespace LiveKitCpp
