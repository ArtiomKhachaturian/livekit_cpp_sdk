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
#include "MediaDevice.h"
#include <api/media_stream_interface.h>
#include <modules/video_capture/video_capture_defines.h>
#include <atomic>

namespace Bricks {
class Logger;
}

namespace rtc {
class Thread;
}

namespace LiveKitCpp
{

class VideoSinkBroadcast;
class CameraCapturer;

class CameraVideoSource : public webrtc::VideoTrackSourceInterface
{
    class Impl;
public:
    CameraVideoSource(std::weak_ptr<rtc::Thread> signalingThread,
                      const webrtc::VideoCaptureCapability& initialCapability = {},
                      const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraVideoSource() override;
    const auto& signalingThread() const noexcept { return _thread; }
    void setDevice(MediaDevice device);
    MediaDevice device() const;
    void setCapability(const webrtc::VideoCaptureCapability& capability);
    webrtc::VideoCaptureCapability capability() const;
    bool enabled() const noexcept { return _enabled; }
    bool setEnabled(bool enabled);
    void setInactive();
    // impl. of webrtc::VideoTrackSourceInterface
    bool is_screencast() const final { return false;}
    std::optional<bool> needs_denoising() const final { return {}; }
    bool GetStats(Stats* stats) final;
    bool SupportsEncodedOutput() const final { return false; }
    void GenerateKeyFrame() final {}
    void AddEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void RemoveEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>*) final {}
    void ProcessConstraints(const webrtc::VideoTrackSourceConstraints& constraints) final;
    // impl. of MediaSourceInterface
    webrtc::MediaSourceInterface::SourceState state() const final;
    bool remote() const final { return false; }
    // impl. of rtc::VideoSourceInterface<VideoFrame>
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) final;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
private:
    template <class Method, typename... Args>
    void postToImpl(Method method, Args&&... args) const;
private:
    const std::weak_ptr<rtc::Thread> _thread;
    const std::shared_ptr<Impl> _impl;
    std::atomic_bool _enabled = true;
};

} // namespace LiveKitCpp
