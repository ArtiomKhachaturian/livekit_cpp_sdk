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
#pragma once // AsyncVideoSource.h
#include "AsyncMediaSource.h"
#include "AsyncVideoSourceImpl.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"

namespace LiveKitCpp
{

class LocalVideoFilterPin;

class AsyncVideoSource : public AsyncMediaSource<webrtc::VideoTrackSourceInterface, AsyncVideoSourceImpl>
{
    using Base = AsyncMediaSource<webrtc::VideoTrackSourceInterface, AsyncVideoSourceImpl>;
public:
    AsyncVideoSource(std::shared_ptr<AsyncVideoSourceImpl> impl);
    void setDeviceInfo(MediaDeviceInfo info);
    MediaDeviceInfo deviceInfo() const;
    void setOptions(VideoOptions options = {});
    VideoOptions options() const;
    void addListener(MediaDeviceListener* listener);
    void removeListener(MediaDeviceListener* listener);
    void setFilter(LocalVideoFilterPin* inputPin);
    VideoContentHint contentHint() const;
    void setContentHint(VideoContentHint hint);
    // impl. of webrtc::VideoTrackSourceInterface
    std::optional<bool> needs_denoising() const final { return {}; }
    bool GetStats(webrtc::VideoTrackSourceInterface::Stats* stats) final;
    bool SupportsEncodedOutput() const final { return false; }
    void GenerateKeyFrame() final {}
    void AddEncodedSink(webrtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void RemoveEncodedSink(webrtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void ProcessConstraints(const webrtc::VideoTrackSourceConstraints& constraints) final;
    // impl. of rtc::VideoSourceInterface<VideoFrame>
    void AddOrUpdateSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const webrtc::VideoSinkWants& wants) final;
    void RemoveSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
};
	
} // namespace LiveKitCpp
