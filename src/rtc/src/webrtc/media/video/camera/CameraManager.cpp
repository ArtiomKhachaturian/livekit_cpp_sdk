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
#include "RtcUtils.h"
#include "VideoUtils.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <modules/video_capture/video_capture_config.h>
#include <cassert>

namespace LiveKitCpp
{

CameraManager::CameraManager(std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> deviceInfo)
    : _deviceInfo(std::move(deviceInfo))
{
}

std::shared_ptr<CameraManager> CameraManager::create()
{
    std::shared_ptr<CameraManager> manager;
    if (auto deviceInfo = createDeviceInfo()) {
        manager.reset(new CameraManager(std::move(deviceInfo)));
    }
    return manager;
}

webrtc::VideoCaptureCapability CameraManager::defaultCapability()
{
    webrtc::VideoCaptureCapability capability;
    // HD ready (720p x 30 fps)
    capability.width = 1280;
    capability.height = 720;
    capability.maxFPS = webrtc::videocapturemodule::kDefaultFrameRate;
#ifdef __APPLE__
    capability.videoType = webrtc::VideoType::kNV12;
#elif defined(WIN32)
    capability.videoType = webrtc::VideoType::kMJPEG;
#else
    capability.videoType = webrtc::VideoType::kI420;
#endif
    return capability;
}

std::string_view CameraManager::logCategory()
{
    static const std::string_view category("camera");
    return category;
}

std::string CameraManager::formatLogMessage(std::string_view deviceName, const std::string& message)
{
    if (!message.empty()) {
        if (!deviceName.empty()) {
            return "'" + std::string(deviceName) + "' - " + message;
        }
    }
    return message;
}

std::string CameraManager::formatLogMessage(const MediaDeviceInfo& info, const std::string& message)
{
    return formatLogMessage(info._name, message);
}

uint32_t CameraManager::devicesNumber() const
{
    return _deviceInfo->NumberOfDevices();
}

bool CameraManager::device(uint32_t number, std::string& name, std::string& guid) const
{
    thread_local static char deviceNameUTF8[webrtc::kVideoCaptureDeviceNameLength] = {0};
    thread_local static char deviceUniqueIdUTF8[webrtc::kVideoCaptureProductIdLength] = {0};
    if (0 == _deviceInfo->GetDeviceName(number, deviceNameUTF8, webrtc::kVideoCaptureDeviceNameLength,
                                        deviceUniqueIdUTF8, webrtc::kVideoCaptureProductIdLength)) {
        name = deviceNameUTF8;
        guid = deviceUniqueIdUTF8;
        return true;
    }
    return false;
}

bool CameraManager::device(uint32_t number, MediaDeviceInfo& out) const
{
    return device(number, out._name, out._guid);
}

bool CameraManager::defaultdevice(uint32_t number, std::string& name, std::string& guid) const
{
    return device(0U, name, guid);
}

bool CameraManager::defaultDevice(MediaDeviceInfo& out) const
{
    return device(0U, out);
}

std::vector<MediaDeviceInfo> CameraManager::devices() const
{
    if (const auto count = devicesNumber()) {
        std::vector<MediaDeviceInfo> devicesInfo;
        devicesInfo.reserve(count);
        for (uint32_t i = 0u; i < count; ++i) {
            MediaDeviceInfo deviceInfo;
            if (device(i, deviceInfo)) {
                devicesInfo.push_back(std::move(deviceInfo));
            }
        }
        return devicesInfo;
    }
    return {};
}

bool CameraManager::deviceIsValid(std::string_view guid) const
{
    if (!guid.empty()) {
        if (const auto count = devicesNumber()) {
            for (uint32_t i = 0u; i < count; ++i) {
                MediaDeviceInfo deviceInfo;
                if (device(i, deviceInfo) && guid == deviceInfo._guid) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool CameraManager::deviceIsValid(const MediaDeviceInfo& info) const
{
    return deviceIsValid(info._guid);
}

uint32_t CameraManager::capabilitiesNumber(std::string_view guid) const
{
    if (!guid.empty()) {
        const auto number = _deviceInfo->NumberOfCapabilities(guid.data()); // signed int
        if (number > 0) {
            return static_cast<uint32_t>(number);
        }
    }
    return 0U;
}

uint32_t CameraManager::capabilitiesNumber(const MediaDeviceInfo& info) const
{
    return capabilitiesNumber(info._guid);
}

bool CameraManager::capability(std::string_view guid, uint32_t number,
                               webrtc::VideoCaptureCapability& capability) const
{
    if (!guid.empty()) {
        return 0 == _deviceInfo->GetCapability(guid.data(), number, capability);
    }
    return false;
}

bool CameraManager::capability(const MediaDeviceInfo& info, uint32_t number,
                               webrtc::VideoCaptureCapability& capability) const
{
    return CameraManager::capability(info._guid, number, capability);
}

bool CameraManager::bestMatchedCapability(std::string_view guid,
                                          const webrtc::VideoCaptureCapability& requested,
                                          webrtc::VideoCaptureCapability& resulting) const
{
    if (!guid.empty()) {
        return _deviceInfo->GetBestMatchedCapability(guid.data(), requested, resulting) >= 0;
    }
    return false;
}

bool CameraManager::bestMatchedCapability(const MediaDeviceInfo& info,
                                          const webrtc::VideoCaptureCapability& requested,
                                          webrtc::VideoCaptureCapability& resulting) const
{
    return bestMatchedCapability(info._guid, requested, resulting);
}

bool CameraManager::orientation(std::string_view guid, webrtc::VideoRotation& orientation) const
{
    if (!guid.empty()) {
        return 0 == _deviceInfo->GetOrientation(guid.data(), orientation);
    }
    return false;
}

bool CameraManager::orientation(const MediaDeviceInfo& info, webrtc::VideoRotation& orientation) const
{
    return CameraManager::orientation(info._guid, orientation);
}

webrtc::scoped_refptr<CameraCapturer> CameraManager::createCapturer(std::string_view guid,
                                                                    VideoFrameBufferPool framesPool,
                                                                    const std::shared_ptr<Bricks::Logger>& logger) const
{
    if (!guid.empty()) {
        if (const uint32_t count = _deviceInfo->NumberOfDevices()) {
            MediaDeviceInfo deviceInfo;
            for (uint32_t i = 0U; i < count; ++i) {
                if (CameraManager::device(i, deviceInfo) && deviceInfo._guid == guid) {
                    return createCapturer(deviceInfo, std::move(framesPool), logger);
                }
            }
        }
    }
    return {};
}

bool CameraManager::displaySettingsDialogBox(std::string_view guid,
                                             std::string_view dialogTitleUTF8,
                                             void* parentWindow,
                                             uint32_t positionX, uint32_t positionY) const
{
    return 0 == _deviceInfo->DisplayCaptureSettingsDialogBox(guid.data(), dialogTitleUTF8.data(),
                                                             parentWindow, positionX, positionY);
}

bool CameraManager::displaySettingsDialogBox(const MediaDeviceInfo& dev,
                                             std::string_view dialogTitleUTF8,
                                             void* parentWindow,
                                             uint32_t positionX, uint32_t positionY) const
{
    return displaySettingsDialogBox(dev._guid, std::move(dialogTitleUTF8), parentWindow, positionX, positionY);
}

std::string toString(const webrtc::VideoCaptureCapability& capability)
{
    return std::to_string(capability.width) + "x" + std::to_string(capability.height) +
            "/" + std::to_string(capability.maxFPS) + "fps|" + 
            fourccToString(webrtc::ConvertVideoType(capability.videoType));
}

VideoOptions map(const webrtc::VideoCaptureCapability& capability)
{
    VideoOptions options;
    options._width = capability.width;
    options._height = capability.height;
    options._maxFPS = capability.maxFPS;
    options._interlaced = capability.interlaced;
    options._type = map(capability.videoType);
    return options;
}

webrtc::VideoCaptureCapability map(const VideoOptions& options)
{
    webrtc::VideoCaptureCapability capability;
    capability.width = options._width;
    capability.height = options._height;
    capability.maxFPS = options._maxFPS;
    capability.interlaced = options._interlaced;
    if (options._type.has_value()) {
        capability.videoType = map(options._type.value());
    }
    return capability;
}

} // namespace LiveKitCpp
