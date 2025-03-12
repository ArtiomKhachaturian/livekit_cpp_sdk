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
#include <api/video/i420_buffer.h>
#include <rtc_base/time_utils.h>

namespace LiveKitCpp
{

CameraCapturer::CameraCapturer(std::string currentDeviceName)
    : _currentDeviceName(std::move(currentDeviceName))
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

std::optional<webrtc::VideoFrame> CameraCapturer::
    createVideoFrame(int srcWidth, int srcHeight, int64_t timeStampMicro,
                     const uint8_t* srcI420Data, uint16_t id,
                     const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (srcWidth > 0 && srcHeight > 0) {
        ::rtc::scoped_refptr<webrtc::I420BufferInterface> dst;
        if (!srcI420Data) {
            dst = webrtc::I420Buffer::Create(srcWidth, srcHeight);
        } else {
            /// see also /webrtc/src/api/video/i420_buffer.cc
            const auto strideY = srcWidth;
            const auto strideU = (srcWidth + 1) / 2, strideV = strideU;
            dst = webrtc::I420Buffer::Copy(srcWidth, srcHeight,
                                           srcI420Data, strideY,
                                           srcI420Data + strideY * srcHeight, strideU,
                                           srcI420Data + strideY * srcHeight + strideU * ((srcHeight + 1) / 2), strideV);
        }
        return createVideoFrame(dst, timeStampMicro, id, colorSpace);
    }
    return std::nullopt;
}

std::optional<webrtc::VideoFrame> CameraCapturer::
    createVideoFrame(const ::rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                     int64_t timeStampMicro, uint16_t id,
                     const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (buff) {
        webrtc::VideoFrame::Builder builder;
        builder.set_video_frame_buffer(buff);
        if (timeStampMicro > 0LL) {
            builder.set_timestamp_us(timeStampMicro);
        } else {
            builder.set_timestamp_us(::rtc::TimeMicros());
        }
        static thread_local uint16_t nextId = 1U;
        if (id > 0) {
            builder.set_id(id);
            nextId = id;
        } else {
            builder.set_id(nextId++);
        }
        auto frame = builder.build();
        frame.set_color_space(colorSpace);
        return frame;
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
