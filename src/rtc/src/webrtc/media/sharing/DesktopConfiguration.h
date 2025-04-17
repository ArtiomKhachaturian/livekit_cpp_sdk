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
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/desktop_geometry.h>
#include <modules/desktop_capture/desktop_capturer.h>
#include <optional>
#include <memory>

namespace LiveKitCpp
{

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
private:
    webrtc::DesktopCapturer* enumerator(bool windows) const;
    static webrtc::DesktopCaptureOptions makeOptions(bool lightweightMode = false);
private:
    const std::unique_ptr<webrtc::DesktopCapturer> _screensEnumerator;
    const std::unique_ptr<webrtc::DesktopCapturer> _windowsEnumerator;
};
	
} // namespace LiveKitCpp
