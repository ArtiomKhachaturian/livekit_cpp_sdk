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
#pragma once // VideoSinkBroadcast.h
#include "VideoFrameBufferPool.h"
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class VideoFrameBufferPoolSource;

class VideoSinkBroadcast : public webrtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    VideoSinkBroadcast(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const webrtc::VideoSinkWants& wants = {});
    void updateSinkWants(const webrtc::VideoSinkWants& wants);
    void setContentHint(VideoContentHint hint);
    // impl. of webrtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) final;
private:
    // Reports the appropriate frame size after adaptation. Returns true
    // if a frame is wanted. Returns false if there are no interested
    // sinks, or if the VideoAdapter decides to drop the frame.
    bool adaptFrame(int width, int height, int64_t timeUs,
                    int& outWidth, int& outHeight,
                    int& cropWidth, int& cropHeight,
                    int& cropX, int& cropY);
    void broadcast(const webrtc::VideoFrame& frame);
private:
    webrtc::VideoSinkInterface<webrtc::VideoFrame>* const _sink;
    const std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    webrtc::VideoAdapter _adapter;
    webrtc::VideoBroadcaster _broadcaster;
    std::atomic_bool _rotationApplied = false;
};

} // namespace LiveKitCpp
