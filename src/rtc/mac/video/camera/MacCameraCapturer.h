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
#pragma once // MacCameraCapturer.h
#include "CameraCapturer.h"
#include <CoreMedia/CMSampleBuffer.h>
#include <list>
#include <optional>
#include <vector>

// forward declarations of ObjC classes
#ifdef __OBJC__
@class AVCaptureDevice;
@class AVCaptureDeviceFormat;
#else
typedef struct objc_object AVCaptureDevice;
typedef struct objc_object AVCaptureDeviceFormat;
#endif

namespace LiveKitCpp
{

class MacCameraCapturer : public CameraCapturer,
                          private rtc::VideoSinkInterface<webrtc::VideoFrame>
{
    class Impl;
public:
    ~MacCameraCapturer() override;
    static rtc::scoped_refptr<MacCameraCapturer> create(const MediaDeviceInfo& deviceInfo,
                                                        VideoFrameBufferPool framesPool = {});
    static AVCaptureDevice* deviceWithUniqueIDUTF8(const char* deviceUniqueIdUTF8);
    static std::vector<webrtc::VideoCaptureCapability> capabilities(AVCaptureDevice* device);
    static std::vector<webrtc::VideoCaptureCapability> capabilities(const char* deviceUniqueIdUTF8);
    static std::string localizedDeviceName(AVCaptureDevice* device);
    static std::string deviceUniqueIdUTF8(AVCaptureDevice* device);
    // impl. of CameraCapturer
    void setObserver(CapturerObserver* observer) final;
    void updateQualityToContentHint() final;
    // impl. of webrtc::VideoCaptureModule
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) final;
    int32_t StopCapture() final;
    int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) final;
    bool CaptureStarted() final;
protected:
    MacCameraCapturer(const MediaDeviceInfo& deviceInfo,
                      VideoFrameBufferPool framesPool,
                      std::unique_ptr<Impl> impl);
private:
    static webrtc::VideoType fromMediaSubType(OSType type);
    static webrtc::VideoType fromMediaSubType(CMFormatDescriptionRef format);
    static bool interlaced(CMFormatDescriptionRef format);
    template <typename Callback>
    static void enumerateFramerates(AVCaptureDeviceFormat* format, Callback callback);
    AVCaptureDeviceFormat* findClosestFormat(const webrtc::VideoCaptureCapability& capability) const;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final { sendFrame(frame); }
    void OnDiscardedFrame() final { discardFrame(); }
private:
    static constexpr int32_t _frameRateStep = 10;
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
