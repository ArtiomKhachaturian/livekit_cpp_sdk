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
#include "CameraCaptureModule.h"
#include "CameraCapturer.h"
#include "MediaDevice.h"
#ifdef WEBRTC_WIN
#include "./Windows/WinCameraCapturer.h"
#include "./Windows/WinCameraPool.h"
#endif

namespace LiveKitCpp
{

struct CameraCaptureModule::Impl
{
    Impl(std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> devInfo);
    const std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> _devInfo;
};

const webrtc::VideoType CameraCaptureModule::_prefferedVideoTypes[] = {
    webrtc::VideoType::kNV12,
    webrtc::VideoType::kI420,
    webrtc::VideoType::kRGB24,
    webrtc::VideoType::kARGB,
    webrtc::VideoType::kBGRA
};

CameraCaptureModule::CameraCaptureModule(std::unique_ptr<Impl> impl)
    : _impl(std::move(impl))
{
}

CameraCaptureModule::~CameraCaptureModule()
{
}

std::shared_ptr<CameraCaptureModule> CameraCaptureModule::create()
{
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> devInfo(createPlatformDeviceInfo());
    if (devInfo) {
        auto impl = std::make_unique<Impl>(std::move(devInfo));
        return std::shared_ptr<CameraCaptureModule>(new CameraCaptureModule(std::move(impl)));
    }
    return {};
}

rtc::scoped_refptr<webrtc::VideoCaptureModule> CameraCaptureModule::createCapturer(const std::string_view& deviceGuid,
                                                                                     CameraObserver* observer) const
{
#ifdef WEBRTC_WIN
    return WinCameraPool::create(deviceGuid, observer);
#else
    if (auto capturer = createPlatformCapturer(deviceGuid)) {
        capturer->setObserver(observer);
        return capturer;
    }
    return nullptr;
#endif
}

uint32_t CameraCaptureModule::numberOfDevices() const
{
    return _impl->_devInfo->NumberOfDevices();
}

uint32_t CameraCaptureModule::numberOfCapabilities(const std::string_view& deviceGuid) const
{
    if (!deviceGuid.empty()) {
        const auto res = _impl->_devInfo->NumberOfCapabilities(deviceGuid.data()); // signed int
        if (res > 0) {
            return static_cast<uint32_t>(res);
        }
    }
    return 0U;
}

std::optional<webrtc::VideoCaptureCapability> CameraCaptureModule::capability(const std::string_view& deviceGuid,
                                                                              uint32_t index) const
{
    if (!deviceGuid.empty()) {
        webrtc::VideoCaptureCapability cap;
        if (0 == _impl->_devInfo->GetCapability(deviceGuid.data(), index, cap)) {
            return cap;
        }
    }
    return std::nullopt;
}

std::optional<webrtc::VideoRotation> CameraCaptureModule::orientation(const std::string_view& deviceGuid, uint32_t index) const
{
    if (!deviceGuid.empty()) {
        webrtc::VideoRotation rotation;
        if (0 == _impl->_devInfo->GetOrientation(deviceGuid.data(), rotation)) {
            return rotation;
        }
    }
    return std::nullopt;
}

std::optional<webrtc::VideoCaptureCapability> CameraCaptureModule::bestMatchedCapability(const std::string_view& deviceGuid,
                                                                                         int32_t width,
                                                                                         int32_t height,
                                                                                         int32_t maxFPS) const
{
    if (!deviceGuid.empty() && width > 0 && height > 0) {
        webrtc::VideoCaptureCapability requested, result;
        requested.width = width;
        requested.height = height;
        requested.maxFPS = maxFPS;
        std::vector<webrtc::VideoCaptureCapability> results;
        results.reserve(std::size(_prefferedVideoTypes));
        for (const auto videoType : _prefferedVideoTypes) {
            requested.videoType = videoType;
            if (-1 != _impl->_devInfo->GetBestMatchedCapability(deviceGuid.data(), requested, result)) {
                if (results.empty() || results.back() != result) {
                    results.push_back(std::move(result));
                }
            }
        }
        if (!results.empty()) {
            std::sort(results.begin(), results.end(), compare);
            return results.back();
        }
    }
    return maxCapability(deviceGuid);
}

std::optional<webrtc::VideoCaptureCapability> CameraCaptureModule::maxCapability(const std::string_view& deviceGuid) const
{
    // Enumerate the supported formats.
    if (const auto numOfCaps = numberOfCapabilities(deviceGuid)) {
        std::optional<webrtc::VideoCaptureCapability> max;
        for (unsigned i = 0; i < numOfCaps; ++i) {
            if (auto current = capability(deviceGuid, i)) {
                if (!max.has_value() || score(current.value()) > score(max.value())) {
                    max = current;
                }
            }
        }
        return max;
    }
    return std::nullopt;
}

bool CameraCaptureModule::device(uint32_t index, std::string& deviceName, std::string& deviceGuid) const
{
    thread_local static char deviceNameUTF8[webrtc::kVideoCaptureDeviceNameLength] = {0};
    thread_local static char deviceUniqueIdUTF8[webrtc::kVideoCaptureProductIdLength] = {0};
    if (0 == _impl->_devInfo->GetDeviceName(index, deviceNameUTF8,
                                            webrtc::kVideoCaptureDeviceNameLength,
                                            deviceUniqueIdUTF8,
                                            webrtc::kVideoCaptureProductIdLength)) {
        deviceName = deviceNameUTF8;
        deviceGuid = deviceUniqueIdUTF8;
        return true;
    }
    return false;
}

bool CameraCaptureModule::device(uint32_t index, MediaDevice& out) const
{
    return device(index, out._name, out._id);
}

uint64_t CameraCaptureModule::score(const webrtc::VideoCaptureCapability& capability)
{
    uint32_t formatScore = 0ULL;
    if (webrtc::VideoType::kUnknown != capability.videoType) {
        auto it = std::find(std::begin(_prefferedVideoTypes), std::end(_prefferedVideoTypes), capability.videoType);
        if (it != std::end(_prefferedVideoTypes)) {
            formatScore = std::size(_prefferedVideoTypes) - std::distance(it, std::begin(_prefferedVideoTypes));
        }
    }
    return formatScore + (capability.width * capability.height * capability.maxFPS);
}

bool CameraCaptureModule::compare(const webrtc::VideoCaptureCapability& l, const webrtc::VideoCaptureCapability& r)
{
    return score(l) < score(r);
}

#ifndef WEBRTC_MAC
bool CameraCaptureModule::hasDevice(const std::string_view& deviceGuid) const
{
    return numberOfCapabilities(deviceGuid) > 0U;
}

webrtc::VideoCaptureModule::DeviceInfo* CameraCaptureModule::createPlatformDeviceInfo()
{
    return webrtc::VideoCaptureFactory::CreateDeviceInfo();
}

rtc::scoped_refptr<CameraCapturer> CameraCaptureModule::createPlatformCapturer(const std::string_view& deviceGuid)
{
#ifdef WEBRTC_WIN
    return WinCameraCapturer::create(deviceGuid);
#else
    DM_ASSERT(false); // not yet implemented
    return nullptr;
#endif
}
#endif

CameraCaptureModule::Impl::Impl(std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> devInfo)
    : _devInfo(std::move(devInfo))
{
}

} // namespace LiveKitCpp
