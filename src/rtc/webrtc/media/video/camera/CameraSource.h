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
#include "AsyncMediaSource.h"
#include "AsyncCameraSourceImpl.h"
#include "media/MediaDeviceInfo.h"
#include <modules/video_capture/video_capture_defines.h>

namespace LiveKitCpp
{

class CameraEventsListener;

class CameraSource : public AsyncMediaSource<webrtc::VideoTrackSourceInterface, AsyncCameraSourceImpl>
{
    using Base = AsyncMediaSource<webrtc::VideoTrackSourceInterface, AsyncCameraSourceImpl>;
public:
    CameraSource(const std::string& id,
                 std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                 const MediaDeviceInfo& info = {},
                 const webrtc::VideoCaptureCapability& initialCapability = {},
                 const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraSource() override;
    void setDeviceInfo(const MediaDeviceInfo& info);
    MediaDeviceInfo deviceInfo() const;
    void setCapability(const webrtc::VideoCaptureCapability& capability);
    webrtc::VideoCaptureCapability capability() const;
    void addListener(CameraEventsListener* listener);
    void removeListener(CameraEventsListener* listener);
    const std::string& id() const;
    // impl. of webrtc::VideoTrackSourceInterface
    bool is_screencast() const final { return false;}
    std::optional<bool> needs_denoising() const final { return {}; }
    bool GetStats(Stats* stats) final;
    bool SupportsEncodedOutput() const final { return false; }
    void GenerateKeyFrame() final {}
    void AddEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void RemoveEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void ProcessConstraints(const webrtc::VideoTrackSourceConstraints& constraints) final;
    // impl. of rtc::VideoSourceInterface<VideoFrame>
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) final;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
};

} // namespace LiveKitCpp
