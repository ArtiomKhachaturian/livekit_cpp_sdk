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
#include "./Camera/CameraCaptureModule.h"
#include "AVCameraCapturer.h"
#include "MacOSCameraCapturer.h"
#include "Utils.h"
#include <modules/video_capture/device_info_impl.h>

namespace
{

class MacOSDeviceInfoImpl : public webrtc::videocapturemodule::DeviceInfoImpl
{
public:
    MacOSDeviceInfoImpl() = default;
    static AVCaptureDevice* findDevice(const char* deviceUniqueIdUTF8);
    // impl. of webrtc::VideoCaptureModule
    uint32_t NumberOfDevices() final;
    int32_t GetDeviceName(uint32_t deviceNumber,
                          char* deviceNameUTF8,
                          uint32_t deviceNameLength,
                          char* deviceUniqueIdUTF8,
                          uint32_t deviceUniqueIdUTF8Length,
                          char* productUniqueIdUTF8 = 0,
                          uint32_t productUniqueIdUTF8Length = 0) final;
    int32_t DisplayCaptureSettingsDialogBox(const char*, const char*, void*,
                                            uint32_t, uint32_t) final { return -1; }
protected:
    // impl. of webrtc::videocapturemodule::DeviceInfoImpl
    int32_t Init() final { return 0; }
    int32_t CreateCapabilityMap(const char* deviceUniqueIdUTF8) final;
};

}

namespace LiveKitCpp
{

bool CameraCaptureModule::hasDevice(const std::string_view& deviceGuid) const
{
    if (!deviceGuid.empty()) {
        @autoreleasepool {
            return nil != MacOSDeviceInfoImpl::findDevice(deviceGuid.data());
        }
    }
    return false;
}

webrtc::VideoCaptureModule::DeviceInfo* CameraCaptureModule::createPlatformDeviceInfo()
{
    return new MacOSDeviceInfoImpl();
}

rtc::scoped_refptr<CameraCapturer> CameraCaptureModule::createPlatformCapturer(const std::string_view& deviceGuid)
{
    return MacOSCameraCapturer::create(deviceGuid);
}

} // namespace LiveKitCpp

namespace
{

using namespace LiveKitCpp;

AVCaptureDevice* MacOSDeviceInfoImpl::findDevice(const char* deviceUniqueIdUTF8)
{
    return [AVCameraCapturer deviceWithUniqueIDUTF8:deviceUniqueIdUTF8];
}

uint32_t MacOSDeviceInfoImpl::NumberOfDevices()
{
    @autoreleasepool {
        auto devs = [AVCameraCapturer availableDevices];
        if (devs) {
            return [devs count];
        }
    }
    return 0U;
}

int32_t MacOSDeviceInfoImpl::GetDeviceName(uint32_t deviceNumber,
                                           char* deviceNameUTF8,
                                           uint32_t deviceNameLength,
                                           char* deviceUniqueIdUTF8,
                                           uint32_t deviceUniqueIdUTF8Length,
                                           char* productUniqueIdUTF8,
                                           uint32_t productUniqueIdUTF8Length)
{
    if (deviceNameUTF8 && deviceNameLength > 0U) {
        @autoreleasepool {
            if (auto devs = [AVCameraCapturer availableDevices]) {
                if (deviceNumber < [devs count]) {
                    auto dev = [devs objectAtIndex:deviceNumber];
                    if (dev) {
                        const auto deviceName = MacOSCameraCapturer::localizedDeviceName(dev);
                        std::strncpy(deviceNameUTF8, deviceName.c_str(), deviceNameLength);
                        if (deviceUniqueIdUTF8 && deviceUniqueIdUTF8Length > 0U) {
                            const auto deviceGuid = fromNSString(dev.uniqueID);
                            std::strncpy(deviceUniqueIdUTF8, deviceGuid.c_str(), deviceUniqueIdUTF8Length);
                        }
                        if (productUniqueIdUTF8 && productUniqueIdUTF8Length > 0U) {
                            const auto model = fromNSString(dev.modelID);
                            std::strncpy(productUniqueIdUTF8, model.c_str(), productUniqueIdUTF8Length);
                        }
                        return 0;
                    }
                }
            }
        }
    }
    return -1;
}

int32_t MacOSDeviceInfoImpl::CreateCapabilityMap(const char* deviceUniqueIdUTF8)
{
    _captureCapabilities = MacOSCameraCapturer::capabilities(deviceUniqueIdUTF8);
    return _captureCapabilities.size();
}

}
