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
#include "./Camera/CameraCapturer.h"
#include <CoreMedia/CMSampleBuffer.h>
#include <list>
#include <optional>
#include <vector>

// forward declarations of ObjC classes
#ifdef __OBJC__
@class AVCaptureDevice;
@class AVCaptureDeviceFormat;
@class AVCameraCapturer;
@class MacOSCameraCapturerDelegate;
#else
typedef struct objc_object AVCaptureDevice;
typedef struct objc_object AVCaptureDeviceFormat;
typedef struct objc_object AVCameraCapturer;
typedef struct objc_object MacOSCameraCapturerDelegate;
#endif

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class MacOSCameraCapturer : public CameraCapturer
{
public:
    ~MacOSCameraCapturer() override;
    static rtc::scoped_refptr<MacOSCameraCapturer>
        create(AVCaptureDevice* device,
               const std::shared_ptr<Bricks::Logger>& logger = {});
    static rtc::scoped_refptr<MacOSCameraCapturer>
        create(const std::string_view& deviceUniqueIdUTF8,
               const std::shared_ptr<Bricks::Logger>& logger = {});
    void deliverFrame(int64_t timestampMicro, CMSampleBufferRef sampleBuffer);
    static std::vector<webrtc::VideoCaptureCapability> capabilities(AVCaptureDevice* device);
    static std::vector<webrtc::VideoCaptureCapability> capabilities(const char* deviceUniqueIdUTF8);
    static std::string localizedDeviceName(AVCaptureDevice* device);
    // impl. of CameraCapturer
    void setObserver(CameraObserver* observer) final;
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
    bool CaptureStarted() final;
protected:
    MacOSCameraCapturer(AVCaptureDevice* device, const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> createBuffer(CMSampleBufferRef sampleBuffer) const;
    AVCaptureDeviceFormat* findClosestFormat(const webrtc::VideoCaptureCapability& capability) const;
    std::optional<webrtc::ColorSpace> activeColorSpace() const;
    static webrtc::VideoType fromMediaSubType(OSType type);
    static webrtc::VideoType fromMediaSubType(CMFormatDescriptionRef format);
    static bool interlaced(CMFormatDescriptionRef format);
    template <typename Callback>
    static void enumerateFramerates(AVCaptureDeviceFormat* format, Callback callback);
private:
    static inline int32_t _frameRateStep = 1;
    AVCaptureDevice* const _device;
    MacOSCameraCapturerDelegate* const _delegate;
    AVCameraCapturer* const _impl;
};

} // namespace LiveKitCpp
