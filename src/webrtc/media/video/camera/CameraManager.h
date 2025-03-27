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
#pragma once // CameraManager.h
#include "CameraOptions.h"
#include <api/scoped_refptr.h>
#include <api/video/video_rotation.h>
#include <modules/video_capture/video_capture_factory.h> // VideoCaptureFactory & VideoCaptureCapability
#include <optional>
#include <memory>
#include <string>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class CameraCapturer;
struct MediaDevice;

class CameraManager
{
public:
    // valid or not
    static bool available();
    // enumeration
    static uint32_t devicesNumber();
    static bool device(uint32_t number, std::string& name, std::string& guid);
    static bool device(uint32_t number, MediaDevice& out);
    static bool defaultdevice(uint32_t number, std::string& name, std::string& guid);
    static bool defaultDevice(MediaDevice& out);
    static std::vector<MediaDevice> devices();
    static bool deviceIsValid(std::string_view guid);
    static bool deviceIsValid(const MediaDevice& device);
    // capabilities API
    static webrtc::VideoCaptureCapability defaultCapability();
    // Returns the number of capabilities this device.
    static uint32_t capabilitiesNumber(std::string_view guid);
    static uint32_t capabilitiesNumber(const MediaDevice& device);
    // Gets the capability of the named device.
    static bool capability(std::string_view guid, uint32_t number,
                           webrtc::VideoCaptureCapability& capability);
    static bool capability(const MediaDevice& device, uint32_t number,
                           webrtc::VideoCaptureCapability& capability);
    static bool bestMatchedCapability(std::string_view guid,
                                      const webrtc::VideoCaptureCapability& requested,
                                      webrtc::VideoCaptureCapability& resulting);
    static bool bestMatchedCapability(const MediaDevice& device,
                                      const webrtc::VideoCaptureCapability& requested,
                                      webrtc::VideoCaptureCapability& resulting);
    // common
    static std::string_view logCategory();
    static std::string formatLogMessage(std::string_view deviceName, const std::string& message);
    static std::string formatLogMessage(const MediaDevice& device, const std::string& message);
    // Gets clockwise angle the captured frames should be rotated in order
    // to be displayed correctly on a normally rotated display.
    static bool orientation(std::string_view guid, webrtc::VideoRotation& orientation);
    static bool orientation(const MediaDevice& device, webrtc::VideoRotation& orientation);
    static rtc::scoped_refptr<CameraCapturer> createCapturer(std::string_view guid,
                                                             const std::shared_ptr<Bricks::Logger>& logger = {});
    static rtc::scoped_refptr<CameraCapturer> createCapturer(const MediaDevice& dev,
                                                             const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    static webrtc::VideoCaptureModule::DeviceInfo* deviceInfo();
};

std::string toString(const webrtc::VideoCaptureCapability& capability);
CameraOptions map(const webrtc::VideoCaptureCapability& capability);
webrtc::VideoCaptureCapability map(const CameraOptions& options);
webrtc::VideoType map(VideoType type);
VideoType map(webrtc::VideoType type);

} // namespace LiveKitCpp
