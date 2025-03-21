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
#include "VideoSinkBroadcast.h"
#include <api/video/i420_buffer.h>

namespace {

inline rtc::scoped_refptr<const webrtc::I420BufferInterface> toI420(const webrtc::VideoFrame& frame) {
    if (const auto buffer = frame.video_frame_buffer()) {
        if (webrtc::VideoFrameBuffer::Type::kI420 == buffer->type()) {
            return rtc::scoped_refptr<const webrtc::I420BufferInterface>(buffer->GetI420());
        }
        return buffer->ToI420();
    }
    return {};
}

}

namespace LiveKitCpp
{

VideoSinkBroadcast::VideoSinkBroadcast(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                       const rtc::VideoSinkWants& wants)
    : _sink(sink)
    , _adapter(2)
{
    assert(_sink);
    updateSinkWants(wants);
}

void VideoSinkBroadcast::updateSinkWants(const rtc::VideoSinkWants& wants)
{
    _broadcaster.AddOrUpdateSink(_sink, wants);
    const auto mixedWants = _broadcaster.wants();
    _adapter.OnSinkWants(mixedWants);
    _rotationApplied = wants.rotation_applied;
}

void VideoSinkBroadcast::applyBlackFrames(bool apply)
{
    auto wants = _broadcaster.wants();
    wants.black_frames = apply;
    updateSinkWants(wants);
}

void VideoSinkBroadcast::OnFrame(const webrtc::VideoFrame& frame)
{
    if (const auto buffer = frame.video_frame_buffer()) {
        int adaptedWidth, adaptedHeight, cropWidth, cropHeight, cropX, cropY;
        if (adaptFrame(frame.width(), frame.height(), frame.timestamp_us(),
                       adaptedWidth, adaptedHeight,
                       cropWidth, cropHeight, cropX, cropY)) {
            if (adaptedWidth == frame.width() && adaptedHeight == frame.height()) {
                // No adaption - optimized path.
                broadcast(frame);
            }
            else if (const auto srcI420 = toI420(frame)) {
                auto dstI420 = webrtc::I420Buffer::Create(adaptedWidth, adaptedHeight);
                dstI420->CropAndScaleFrom(*srcI420, cropX, cropY, cropWidth, cropHeight);
                webrtc::VideoFrame adapted(frame);
                adapted.set_video_frame_buffer(dstI420);
                broadcast(adapted);
            }
        }
    }
}

void VideoSinkBroadcast::OnDiscardedFrame()
{
    _broadcaster.OnDiscardedFrame();
}

void VideoSinkBroadcast::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints)
{
    _broadcaster.ProcessConstraints(constraints);
    _sink->OnConstraintsChanged(constraints);
}

bool VideoSinkBroadcast::adaptFrame(int width, int height, int64_t timeUs,
                                   int& outWidth, int& outHeight,
                                   int& cropWidth, int& cropHeight,
                                   int& cropX, int& cropY)
{
    if (!_adapter.AdaptFrameResolution(width, height,
                                       timeUs * rtc::kNumNanosecsPerMicrosec,
                                       &cropWidth, &cropHeight,
                                       &outWidth, &outHeight)) {
        _broadcaster.OnDiscardedFrame();
        // VideoAdapter dropped the frame.
        return false;
    }

    cropX = (width - cropWidth) / 2;
    cropY = (height - cropHeight) / 2;
    return true;
}

void VideoSinkBroadcast::broadcast(const webrtc::VideoFrame& frame)
{
    if (_rotationApplied && frame.rotation() != webrtc::kVideoRotation_0) {
        if (const auto srcI420 = toI420(frame)) {
            webrtc::VideoFrame rotatedFrame(frame);
            auto rotatedBuffer = webrtc::I420Buffer::Rotate(*srcI420, frame.rotation());
            rotatedFrame.set_video_frame_buffer(rotatedBuffer);
            rotatedFrame.set_rotation(webrtc::kVideoRotation_0);
            _broadcaster.OnFrame(rotatedFrame);
        }
    } else {
        _broadcaster.OnFrame(frame);
    }
}

} // namespace LiveKitCpp
