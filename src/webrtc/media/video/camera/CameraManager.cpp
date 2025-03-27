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
#include "Utils.h"
#include <modules/video_capture/video_capture_config.h>
#include <cassert>

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
        thread_local static char deviceNameUTF8[webrtc::kVideoCaptureDeviceNameLength] = {0};
        thread_local static char deviceUniqueIdUTF8[webrtc::kVideoCaptureProductIdLength] = {0};
        if (0 == di->GetDeviceName(number, deviceNameUTF8, webrtc::kVideoCaptureDeviceNameLength,
                                   deviceUniqueIdUTF8, webrtc::kVideoCaptureProductIdLength)) {
            name = deviceNameUTF8;
            guid = deviceUniqueIdUTF8;
            return true;
        }
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

bool CameraManager::deviceIsValid(std::string_view guid)
{
    if (!guid.empty()) {
        if (const auto count = devicesNumber()) {
            for (uint32_t i = 0u; i < count; ++i) {
                MediaDevice deviceInfo;
                if (device(i, deviceInfo) && guid == deviceInfo._guid) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool CameraManager::deviceIsValid(const MediaDevice& device)
{
    return deviceIsValid(device._guid);
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
            return 0 == di->GetCapability(guid.data(), number, capability);
        }
    }
    return false;
}

bool CameraManager::capability(const MediaDevice& device, uint32_t number,
                                   webrtc::VideoCaptureCapability& capability)
{
    return CameraManager::capability(device._guid, number, capability);
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

std::string CameraManager::formatLogMessage(std::string_view deviceName, const std::string& message)
{
    if (!message.empty()) {
        if (!deviceName.empty()) {
            return "'" + std::string(deviceName) + "' - " + message;
        }
    }
    return message;
}

std::string CameraManager::formatLogMessage(const MediaDevice& device, const std::string& message)
{
    return formatLogMessage(device._name, message);
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

std::string toString(const webrtc::VideoCaptureCapability& capability)
{
    return std::to_string(capability.width) + "x" + std::to_string(capability.height) +
            "/" + std::to_string(capability.maxFPS) + "fps|" + 
            fourccToString(webrtc::ConvertVideoType(capability.videoType));
}

CameraOptions map(const webrtc::VideoCaptureCapability& capability)
{
    CameraOptions options;
    options._width = capability.width;
    options._height = capability.height;
    options._maxFPS = capability.maxFPS;
    options._interlaced = capability.interlaced;
    options._type = map(capability.videoType);
    return options;
}

webrtc::VideoCaptureCapability map(const CameraOptions& options)
{
    webrtc::VideoCaptureCapability capability;
    capability.width = options._width;
    capability.height = options._height;
    capability.maxFPS = options._maxFPS;
    capability.interlaced = options._interlaced;
    capability.videoType = map(options._type);
    return capability;
}

webrtc::VideoType map(VideoType type)
{
    switch (type) {
        case VideoType::Unknown:
            break;
        case VideoType::I420:
            return webrtc::VideoType::kI420;
        case VideoType::IYUV:
            return webrtc::VideoType::kIYUV;
        case VideoType::RGB24:
            return webrtc::VideoType::kRGB24;
        case VideoType::BGR24:
            return webrtc::VideoType::kBGR24;
        case VideoType::ARGB:
            return webrtc::VideoType::kARGB;
        case VideoType::ABGR:
            return webrtc::VideoType::kABGR;
        case VideoType::RGB565:
            return webrtc::VideoType::kRGB565;
        case VideoType::YUY2:
            return webrtc::VideoType::kYUY2;
        case VideoType::YV12:
            return webrtc::VideoType::kYV12;
        case VideoType::UYVY:
            return webrtc::VideoType::kUYVY;
        case VideoType::MJPEG:
            return webrtc::VideoType::kMJPEG;
        case VideoType::BGRA:
            return webrtc::VideoType::kBGRA;
        case VideoType::NV12:
            return webrtc::VideoType::kNV12;
        default:
            assert(false);
            break;
    }
    return webrtc::VideoType::kUnknown;
}

VideoType map(webrtc::VideoType type)
{
    switch (type) {
        case webrtc::VideoType::kUnknown:
            break;
        case webrtc::VideoType::kI420:
            return VideoType::I420;
        case webrtc::VideoType::kIYUV:
            return VideoType::IYUV;
        case webrtc::VideoType::kRGB24:
            return VideoType::RGB24;
        case webrtc::VideoType::kBGR24:
            return VideoType::BGR24;
        case webrtc::VideoType::kARGB:
            return VideoType::ARGB;
        case webrtc::VideoType::kABGR:
            return VideoType::ABGR;
        case webrtc::VideoType::kRGB565:
            return VideoType::RGB565;
        case webrtc::VideoType::kYUY2:
            return VideoType::YUY2;
        case webrtc::VideoType::kYV12:
            return VideoType::YV12;
        case webrtc::VideoType::kUYVY:
            return VideoType::UYVY;
        case webrtc::VideoType::kMJPEG:
            return VideoType::MJPEG;
        case webrtc::VideoType::kBGRA:
            return VideoType::BGRA;
        case webrtc::VideoType::kNV12:
            return VideoType::NV12;
        default:
            assert(false);
            break;
    }
    return VideoType::Unknown;
}

} // namespace LiveKitCpp
