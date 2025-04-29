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
#include "Utils.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
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

bool CameraManager::device(uint32_t number, MediaDeviceInfo& out)
{
    return device(number, out._name, out._guid);
}

bool CameraManager::defaultdevice(uint32_t number, std::string& name, std::string& guid)
{
    return device(0U, name, guid);
}

bool CameraManager::defaultDevice(MediaDeviceInfo& out)
{
    return device(0U, out);
}

std::vector<MediaDeviceInfo> CameraManager::devices()
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

bool CameraManager::deviceIsValid(std::string_view guid)
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

bool CameraManager::deviceIsValid(const MediaDeviceInfo& info)
{
    return deviceIsValid(info._guid);
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

uint32_t CameraManager::capabilitiesNumber(const MediaDeviceInfo& info)
{
    return capabilitiesNumber(info._guid);
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

bool CameraManager::capability(const MediaDeviceInfo& info, uint32_t number,
                               webrtc::VideoCaptureCapability& capability)
{
    return CameraManager::capability(info._guid, number, capability);
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

bool CameraManager::bestMatchedCapability(const MediaDeviceInfo& info,
                                          const webrtc::VideoCaptureCapability& requested,
                                          webrtc::VideoCaptureCapability& resulting)
{
    return bestMatchedCapability(info._guid, requested, resulting);
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

bool CameraManager::orientation(std::string_view guid, webrtc::VideoRotation& orientation)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            return 0 == di->GetOrientation(guid.data(), orientation);
        }
    }
    return false;
}

bool CameraManager::orientation(const MediaDeviceInfo& info, webrtc::VideoRotation& orientation)
{
    return CameraManager::orientation(info._guid, orientation);
}

rtc::scoped_refptr<CameraCapturer> CameraManager::createCapturer(std::string_view guid,
                                                                 VideoFrameBufferPool framesPool,
                                                                 const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!guid.empty()) {
        if (const auto di = deviceInfo()) {
            if (const uint32_t count = di->NumberOfDevices()) {
                MediaDeviceInfo deviceInfo;
                for (uint32_t i = 0U; i < count; ++i) {
                    if (CameraManager::device(i, deviceInfo) && deviceInfo._guid == guid) {
                        return createCapturer(deviceInfo, std::move(framesPool), logger);
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

webrtc::VideoType map(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::I420:
            return webrtc::VideoType::kI420;
        case VideoFrameType::IYUV:
            return webrtc::VideoType::kIYUV;
        case VideoFrameType::RGB24:
            return webrtc::VideoType::kRGB24;
        case VideoFrameType::BGR24:
            return webrtc::VideoType::kBGR24;
        case VideoFrameType::ARGB32:
            return webrtc::VideoType::kARGB;
        case VideoFrameType::ABGR32:
            return webrtc::VideoType::kABGR;
        case VideoFrameType::RGB565:
            return webrtc::VideoType::kRGB565;
        case VideoFrameType::YUY2:
            return webrtc::VideoType::kYUY2;
        case VideoFrameType::YV12:
            return webrtc::VideoType::kYV12;
        case VideoFrameType::UYVY:
            return webrtc::VideoType::kUYVY;
        case VideoFrameType::MJPEG:
            return webrtc::VideoType::kMJPEG;
        case VideoFrameType::BGRA32:
            return webrtc::VideoType::kBGRA;
        case VideoFrameType::NV12:
            return webrtc::VideoType::kNV12;
        default:
            break;
    }
    return webrtc::VideoType::kUnknown;
}

std::optional<VideoFrameType> map(webrtc::VideoType type)
{
    switch (type) {
        case webrtc::VideoType::kUnknown:
            break;
        case webrtc::VideoType::kI420:
            return VideoFrameType::I420;
        case webrtc::VideoType::kIYUV:
            return VideoFrameType::IYUV;
        case webrtc::VideoType::kRGB24:
            return VideoFrameType::RGB24;
        case webrtc::VideoType::kBGR24:
            return VideoFrameType::BGR24;
        case webrtc::VideoType::kARGB:
            return VideoFrameType::ARGB32;
        case webrtc::VideoType::kABGR:
            return VideoFrameType::ABGR32;
        case webrtc::VideoType::kRGB565:
            return VideoFrameType::RGB565;
        case webrtc::VideoType::kYUY2:
            return VideoFrameType::YUY2;
        case webrtc::VideoType::kYV12:
            return VideoFrameType::YV12;
        case webrtc::VideoType::kUYVY:
            return VideoFrameType::UYVY;
        case webrtc::VideoType::kMJPEG:
            return VideoFrameType::MJPEG;
        case webrtc::VideoType::kBGRA:
            return VideoFrameType::BGRA32;
        case webrtc::VideoType::kNV12:
            return VideoFrameType::NV12;
        default:
            assert(false);
            break;
    }
    return {};
}

} // namespace LiveKitCpp
