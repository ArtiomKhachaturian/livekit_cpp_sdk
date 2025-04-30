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
#include "SafeObj.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_config.h>
#include <atomic>
#include <memory>
#include <string>

namespace LiveKitCpp
{

class CapturerObserver;

class CameraCapturer : public webrtc::VideoCaptureModule
{
public:
    ~CameraCapturer() override;
    virtual void updateQualityToContentHint() {}
    virtual void setObserver(CapturerObserver* /*observer*/ = nullptr) {}
    const auto& guid() const noexcept { return _deviceInfo._guid; }
    // impl. of webrtc::VideoCaptureModule
    void RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
    void RegisterCaptureDataCallback(webrtc::RawVideoSinkInterface* rawSink) final;
    void DeRegisterCaptureDataCallback() final;
    int32_t SetCaptureRotation(webrtc::VideoRotation rotation) final;
    bool SetApplyRotation(bool enable) final;
    bool GetApplyRotation() final { return _applyRotation; }
    const char* CurrentDeviceName() const final { return _deviceInfo._name.c_str(); }
protected:
    CameraCapturer(const MediaDeviceInfo& deviceInfo);
    bool hasSink() const { return nullptr != _sink(); }
    bool hasRawSink() const { return nullptr != _rawSink(); }
    virtual bool doApplyRotation(bool enable);
    bool sendFrame(const webrtc::VideoFrame& frame);
    int32_t sendRawFrame(uint8_t* videoFrame, size_t videoFrameLength,
                         const webrtc::VideoCaptureCapability& frameInfo,
                         webrtc::VideoRotation rotation,
                         int64_t captureTime);
    void discardFrame();
    webrtc::VideoRotation captureRotation() const;
private:
    const MediaDeviceInfo _deviceInfo;
    Bricks::SafeObj<rtc::VideoSinkInterface<webrtc::VideoFrame>*> _sink = nullptr;
    Bricks::SafeObj<webrtc::RawVideoSinkInterface*> _rawSink = nullptr;
    // set if the frame should be rotated by the capture module
    std::atomic<webrtc::VideoRotation> _rotateFrame = webrtc::kVideoRotation_0;
    // indicate whether rotation should be applied before delivered externally
    std::atomic_bool _applyRotation = false;
};

} // namespace LiveKitCpp
