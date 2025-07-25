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
#include "Utils.h"

namespace
{
const std::string g_screenMarker = "!eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9<";
const std::string g_windowMarker = "!ikUaGOaxzKw0JlNj080oEkPG2S42GIck3O65<";

}

namespace LiveKitCpp
{

std::string windowIdToString(webrtc::WindowId id)
{
    return g_windowMarker + toHexValue(id);
}

std::string screenIdToString(webrtc::ScreenId id)
{
    return g_screenMarker + toHexValue(id);
}

std::optional<webrtc::WindowId> windowIdFromString(const std::string& str)
{
    if (startWith(str, g_windowMarker)) {
        return fromHexValue<webrtc::WindowId>(str.substr(g_windowMarker.size()));
    }
    return std::nullopt;
}

std::optional<webrtc::ScreenId> screenIdFromString(const std::string& str)
{
    if (startWith(str, g_screenMarker)) {
        return fromHexValue<webrtc::ScreenId>(str.substr(g_screenMarker.size()));
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
