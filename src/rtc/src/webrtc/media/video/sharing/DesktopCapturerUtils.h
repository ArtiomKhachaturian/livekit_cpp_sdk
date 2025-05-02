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
#pragma once // DesktopCapturerUtils.h
#include <modules/desktop_capture/desktop_capture_types.h>
#include <modules/desktop_capture/desktop_capture_options.h>
#include <string>
#include <optional>
#include <vector>

namespace LiveKitCpp
{

// helper routines fow working with common WebRTC sharing types
bool screenExists(webrtc::ScreenId sId);
bool windowExists(webrtc::WindowId wId);
std::string screenTitle(webrtc::ScreenId sId);
std::optional<std::string> windowTitle(webrtc::WindowId wId);
webrtc::DesktopSize screenResolution(const webrtc::DesktopCaptureOptions& options, webrtc::ScreenId sId);
bool enumerateScreens(const webrtc::DesktopCaptureOptions& options, std::vector<std::string>& out);
bool enumerateWindows(const webrtc::DesktopCaptureOptions& options, std::vector<std::string>& out);
std::string windowIdToString(webrtc::WindowId id);
std::string screenIdToString(webrtc::ScreenId id);
std::optional<webrtc::WindowId> windowIdFromString(const std::string& str);
std::optional<webrtc::ScreenId> screenIdFromString(const std::string& str);
#ifdef WEBRTC_WIN
// unfortunately standard webrtc::IsWgcSupported doesn't work properly 
// for applications without manifest, because rtc::rtc_win::GetVersion() returns incorrect OS version
bool isWgcSupported(bool window);
#endif
	
} // namespace LiveKitCpp
