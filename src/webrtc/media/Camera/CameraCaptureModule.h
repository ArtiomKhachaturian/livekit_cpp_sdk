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
#pragma once
#include <api/scoped_refptr.h>
#include <api/video/video_rotation.h>
#include <memory>
#include <modules/video_capture/video_capture_factory.h> // VideoCaptureFactory & VideoCaptureCapability
#include <optional>
#include <string>

namespace LiveKitCpp
{

class CameraObserver;
class CameraCapturer;
struct MediaDevice;

class CameraCaptureModule
{
    struct Impl;

public:
    ~CameraCaptureModule();
    static std::shared_ptr<CameraCaptureModule> create();
    rtc::scoped_refptr<webrtc::VideoCaptureModule>
        createCapturer(const std::string_view& deviceGuid, CameraObserver* observer = nullptr) const;
    uint32_t numberOfDevices() const;
    uint32_t numberOfCapabilities(const std::string_view& deviceGuid) const;
    bool hasDevice(const std::string_view& deviceGuid) const;
    std::optional<webrtc::VideoCaptureCapability> capability(const std::string_view& deviceGuid,
                                                             uint32_t index) const;
    std::optional<webrtc::VideoRotation> orientation(const std::string_view& deviceGuid,
                                                     uint32_t index) const;
    std::optional<webrtc::VideoCaptureCapability>
        bestMatchedCapability(const std::string_view& deviceGuid,
                              int32_t width, int32_t height,
                              int32_t maxFPS) const;
    std::optional<webrtc::VideoCaptureCapability> maxCapability(const std::string_view& deviceGuid) const;
    bool device(uint32_t index, std::string& deviceName, std::string& deviceGuid) const;
    bool device(uint32_t index, MediaDevice& out) const;
protected:
    CameraCaptureModule(std::unique_ptr<Impl> impl);
private:
    static uint64_t score(const webrtc::VideoCaptureCapability& capability);
    static bool compare(const webrtc::VideoCaptureCapability& l, const webrtc::VideoCaptureCapability& r);
    static webrtc::VideoCaptureModule::DeviceInfo* createPlatformDeviceInfo();
    static rtc::scoped_refptr<CameraCapturer> createPlatformCapturer(const std::string_view& deviceGuid);
private:
    static const webrtc::VideoType _prefferedVideoTypes[];
    const std::unique_ptr<Impl> _impl;
};


} // namespace LiveKitCpp
