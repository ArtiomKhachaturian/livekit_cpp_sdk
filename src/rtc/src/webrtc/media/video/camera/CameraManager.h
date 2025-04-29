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
#include "VideoFrameBufferPool.h"
#include "livekit/rtc/media/VideoOptions.h"
#include <api/scoped_refptr.h>
#include <api/video/video_rotation.h>
#include <modules/video_capture/video_capture_factory.h> // VideoCaptureFactory & VideoCaptureCapability
#include <memory>
#include <optional>
#include <string>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class CameraCapturer;
struct MediaDeviceInfo;

class CameraManager
{
public:
    static std::shared_ptr<CameraManager> create();
    static webrtc::VideoCaptureCapability defaultCapability();
    static std::string_view logCategory();
    static std::string formatLogMessage(std::string_view deviceName, const std::string& message);
    static std::string formatLogMessage(const MediaDeviceInfo& info, const std::string& message);
    // enumeration
    uint32_t devicesNumber() const;
    bool device(uint32_t number, std::string& name, std::string& guid) const;
    bool device(uint32_t number, MediaDeviceInfo& out) const;
    bool defaultdevice(uint32_t number, std::string& name, std::string& guid) const;
    bool defaultDevice(MediaDeviceInfo& out) const;
    std::vector<MediaDeviceInfo> devices() const;
    bool deviceIsValid(std::string_view guid) const;
    bool deviceIsValid(const MediaDeviceInfo& info) const;
    // capabilities API
    // Returns the number of capabilities this device.
    uint32_t capabilitiesNumber(std::string_view guid) const;
    uint32_t capabilitiesNumber(const MediaDeviceInfo& info) const;
    // Gets the capability of the named device.
    bool capability(std::string_view guid, uint32_t number,
                    webrtc::VideoCaptureCapability& capability) const;
    bool capability(const MediaDeviceInfo& info, uint32_t number,
                    webrtc::VideoCaptureCapability& capability) const;
    bool bestMatchedCapability(std::string_view guid,
                               const webrtc::VideoCaptureCapability& requested,
                               webrtc::VideoCaptureCapability& resulting) const;
    bool bestMatchedCapability(const MediaDeviceInfo& info,
                               const webrtc::VideoCaptureCapability& requested,
                               webrtc::VideoCaptureCapability& resulting) const;
    // common
    // Gets clockwise angle the captured frames should be rotated in order
    // to be displayed correctly on a normally rotated display.
    bool orientation(std::string_view guid, webrtc::VideoRotation& orientation) const;
    bool orientation(const MediaDeviceInfo& info, webrtc::VideoRotation& orientation) const;
    rtc::scoped_refptr<CameraCapturer> createCapturer(std::string_view guid,
                                                      VideoFrameBufferPool framesPool = {},
                                                      const std::shared_ptr<Bricks::Logger>& logger = {}) const;
    rtc::scoped_refptr<CameraCapturer> createCapturer(const MediaDeviceInfo& dev,
                                                      VideoFrameBufferPool framesPool = {},
                                                      const std::shared_ptr<Bricks::Logger>& logger = {}) const;
private:
    CameraManager(std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> deviceInfo);
    static std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> createDeviceInfo();
private:
    const std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> _deviceInfo;
};

std::string toString(const webrtc::VideoCaptureCapability& capability);
VideoOptions map(const webrtc::VideoCaptureCapability& capability);
webrtc::VideoCaptureCapability map(const VideoOptions& options);

} // namespace LiveKitCpp
