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
#include "CameraManager.h"
#include "CameraCapturer.h"
#include "MediaDevice.h"
#include <modules/video_capture/video_capture_config.h>

namespace LiveKitCpp
{

bool CameraManager::available()
{
    return nullptr != deviceInfo();
}

uint32_t CameraManager::devicesNumber()
{
    if (const auto di = deviceInfo()) {
        return di->NumberOfDevices();
    }
    return 0U;
}

bool CameraManager::device(uint32_t number, std::string& name, std::string& guid)
{
    if (const auto di = deviceInfo()) {
        name.resize(webrtc::kVideoCaptureDeviceNameLength, 0);
        guid.resize(webrtc::kVideoCaptureProductIdLength, 0);
        return 0 == di->GetDeviceName(number,
                                      name.data(), webrtc::kVideoCaptureDeviceNameLength,
                                      guid.data(), webrtc::kVideoCaptureProductIdLength);
    }
    return false;
}

bool CameraManager::device(uint32_t number, MediaDevice& out)
{
    return device(number, out._name, out._guid);
}

bool CameraManager::defaultdevice(uint32_t number, std::string& name, std::string& guid)
{
    return device(0U, name, guid);
}

bool CameraManager::defaultDevice(MediaDevice& out)
{
    return device(0U, out);
}

std::vector<MediaDevice> CameraManager::devices()
{
    if (const auto count = devicesNumber()) {
        std::vector<MediaDevice> devicesInfo;
        devicesInfo.reserve(count);
        for (uint32_t i = 0u; i < count; ++i) {
            MediaDevice deviceInfo;
            if (device(i, deviceInfo)) {
                devicesInfo.push_back(std::move(deviceInfo));
            }
        }
        return devicesInfo;
    }
    return {};
}

webrtc::VideoCaptureCapability CameraManager::defaultCapability()
{
    webrtc::VideoCaptureCapability capability;
    capability.width = webrtc::videocapturemodule::kDefaultWidth;
    capability.height = webrtc::videocapturemodule::kDefaultHeight;
    capability.maxFPS = 30;
#ifdef __APPLE__
    capability.videoType = webrtc::VideoType::kNV12;
#else
    capability.videoType = webrtc::VideoType::kI420;
#endif
    return capability;
}

uint32_t CameraManager::capabilitiesNumber(std::string_view guid)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            const auto number = di->NumberOfCapabilities(guid.data()); // signed int
            if (number > 0) {
                return static_cast<uint32_t>(number);
            }
        }
    }
    return 0U;
}

uint32_t CameraManager::capabilitiesNumber(const MediaDevice& device)
{
    return capabilitiesNumber(device._guid);
}

bool CameraManager::capability(std::string_view guid,
                               uint32_t number,
                               webrtc::VideoCaptureCapability& capability)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            return di->GetCapability(guid.data(), number, capability);
        }
    }
    return false;
}

uint32_t CameraManager::capability(const MediaDevice& device, uint32_t number,
                                   webrtc::VideoCaptureCapability& capability)
{
    return CameraManager::capability(device._guid, number, capability);
}

std::vector<webrtc::VideoCaptureCapability> CameraManager::capabilities(std::string_view guid)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            const auto number = di->NumberOfCapabilities(guid.data());
            if (number > 0) {
                std::vector<webrtc::VideoCaptureCapability> capabilities;
                capabilities.reserve(number);
                for (uint32_t i = 0; i < number; ++i) {
                    webrtc::VideoCaptureCapability capability;
                    if (0 == di->GetCapability(guid.data(), i, capability)) {
                        capabilities.push_back(std::move(capability));
                    }
                }
                return capabilities;
            }
        }
    }
    return {};
}

std::vector<webrtc::VideoCaptureCapability> CameraManager::capabilities(const MediaDevice& device)
{
    return capabilities(device._guid);
}

bool CameraManager::bestMatchedCapability(std::string_view guid,
                                          const webrtc::VideoCaptureCapability& requested,
                                          webrtc::VideoCaptureCapability& resulting)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            return di->GetBestMatchedCapability(guid.data(), requested, resulting) >= 0;
        }
    }
    return false;
}

bool CameraManager::bestMatchedCapability(const MediaDevice& device,
                                          const webrtc::VideoCaptureCapability& requested,
                                          webrtc::VideoCaptureCapability& resulting)
{
    return bestMatchedCapability(device._guid, requested, resulting);
}

std::string_view CameraManager::logCategory()
{
    static const std::string_view category("camera");
    return category;
}

bool CameraManager::orientation(std::string_view guid, webrtc::VideoRotation& orientation)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            return 0 == di->GetOrientation(guid.data(), orientation);
        }
    }
    return false;
}

bool CameraManager::orientation(const MediaDevice& device, webrtc::VideoRotation& orientation)
{
    return CameraManager::orientation(device._guid, orientation);
}

rtc::scoped_refptr<CameraCapturer> CameraManager::createCapturer(std::string_view guid,
                                                                 const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            if (const uint32_t count = di->NumberOfDevices()) {
                MediaDevice deviceInfo;
                for (uint32_t i = 0U; i < count; ++i) {
                    if (CameraManager::device(i, deviceInfo) && deviceInfo._guid == guid) {
                        return createCapturer(deviceInfo, logger);
                    }
                }
            }
        }
    }
    return {};
}

} // namespace LiveKitCpp
