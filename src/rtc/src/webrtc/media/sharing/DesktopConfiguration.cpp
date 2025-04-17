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
#include "Utils.h"

namespace LiveKitCpp
{

DesktopConfiguration::DesktopConfiguration()
    : _screensEnumerator(webrtc::DesktopCapturer::CreateScreenCapturer(makeOptions(false)))
    , _windowsEnumerator(webrtc::DesktopCapturer::CreateWindowCapturer(makeOptions(true)))
{
}

DesktopConfiguration::~DesktopConfiguration()
{
}

webrtc::DesktopSize DesktopConfiguration::screenResolution(const MediaDeviceInfo& /*dev*/) const
{
    return {};
}

std::vector<MediaDeviceInfo> DesktopConfiguration::enumerate(bool windows) const
{
    std::vector<MediaDeviceInfo> devices;
    if (const auto e = enumerator(windows)) {
        webrtc::DesktopCapturer::SourceList sources;
        if (e->GetSourceList(&sources)) {
            devices.reserve(sources.size());
            for (auto& source : sources) {
                MediaDeviceInfo info;
                info._name = std::move(source.title);
                if (info._name.empty()) {
                    info._name = windows ? windowTitle(source.id) : screenTitle(source.id);
                }
                info._guid = std::to_string(source.id);
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

webrtc::DesktopCapturer* DesktopConfiguration::enumerator(bool windows) const
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
    if (lightweightMode) {
        options.set_full_screen_window_detector(nullptr);
        options.set_configuration_monitor(nullptr);
    }
#endif
    return options;
}

} // namespace LiveKitCpp
