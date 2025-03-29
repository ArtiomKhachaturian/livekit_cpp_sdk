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
#include "CameraCapturer.h"

namespace LiveKitCpp
{

CameraCapturer::CameraCapturer(const MediaDeviceInfo& deviceInfo)
    : _deviceInfo(deviceInfo)
{
}

CameraCapturer::~CameraCapturer()
{
    DeRegisterCaptureDataCallback();
}

void CameraCapturer::RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    _sink(sink);
}

void CameraCapturer::RegisterCaptureDataCallback(webrtc::RawVideoSinkInterface* rawSink)
{
    _rawSink(rawSink);
}

void CameraCapturer::DeRegisterCaptureDataCallback()
{
    _sink(nullptr);
    _rawSink(nullptr);
}

int32_t CameraCapturer::SetCaptureRotation(webrtc::VideoRotation rotation)
{
    _rotateFrame = rotation;
    return 0;
}

bool CameraCapturer::SetApplyRotation(bool enable)
{
    if (doApplyRotation(enable)) {
        _applyRotation = enable;
        return true;
    }
    return false;
}

bool CameraCapturer::doApplyRotation(bool enable)
{
    return enable == false;
}

bool CameraCapturer::sendFrame(const webrtc::VideoFrame& frame)
{
    LOCK_READ_SAFE_OBJ(_sink);
    if (const auto& sink = _sink.constRef()) {
        sink->OnFrame(frame);
        return true;
    }
    return false;
}

int32_t CameraCapturer::sendRawFrame(uint8_t* videoFrame, size_t videoFrameLength,
                                     const webrtc::VideoCaptureCapability& frameInfo,
                                     webrtc::VideoRotation rotation,
                                     int64_t captureTime)
{
    LOCK_READ_SAFE_OBJ(_rawSink);
    if (const auto& rawSink = _rawSink.constRef()) {
        return rawSink->OnRawFrame(videoFrame, videoFrameLength, frameInfo,
                                   rotation, captureTime);
    }
    return -1;
}

void CameraCapturer::discardFrame()
{
    LOCK_READ_SAFE_OBJ(_sink);
    if (const auto& sink = _sink.constRef()) {
        sink->OnDiscardedFrame();
    }
}

webrtc::VideoRotation CameraCapturer::captureRotation() const
{
    if (_applyRotation.load(std::memory_order_relaxed)) {
        return _rotateFrame.load(std::memory_order_relaxed);
    }
    return webrtc::VideoRotation::kVideoRotation_0;
}

} // namespace LiveKitCpp
