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
#pragma once // VideoAdapter.h
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <atomic>

namespace LiveKitCpp
{

class VideoFrameBufferPool;

class VideoSinkBroadcast
{
public:
    VideoSinkBroadcast(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants = {});
    void updateSinkWants(const rtc::VideoSinkWants& wants);
    void deliverFrame(const webrtc::VideoFrame& frame, const VideoFrameBufferPool& framesPool);
    void discardFrame();
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
private:
    // Reports the appropriate frame size after adaptation. Returns true
    // if a frame is wanted. Returns false if there are no interested
    // sinks, or if the VideoAdapter decides to drop the frame.
    bool adaptFrame(int width, int height, int64_t timeUs,
                    int& outWidth, int& outHeight,
                    int& cropWidth, int& cropHeight,
                    int& cropX, int& cropY);
    void broadcast(const webrtc::VideoFrame& frame, const VideoFrameBufferPool& framesPool);
private:
    rtc::VideoSinkInterface<webrtc::VideoFrame>* const _sink;
    cricket::VideoAdapter _adapter;
    rtc::VideoBroadcaster _broadcaster;
    std::atomic_bool _rotationApplied = false;
};

} // namespace LiveKitCpp
