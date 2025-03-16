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
#include "AsyncListeners.h"
#include "CameraCapturerProxySink.h"
#include "Loggable.h"
#include "MediaDevice.h"
#include "SafeScopedRefPtr.h"
#include <api/media_stream_interface.h>
#include <modules/video_capture/video_capture_defines.h>
#include <atomic>
#include <memory>
#include <unordered_map>

namespace rtc {
class Thread;
}

namespace LiveKitCpp
{

class VideoSinkBroadcast;
class CameraCapturer;

class CameraVideoSource : public webrtc::VideoTrackSourceInterface,
                          private Bricks::LoggableS<CameraCapturerProxySink>
{
    using Broadcasters = std::unordered_map<rtc::VideoSinkInterface<webrtc::VideoFrame>*,
                                            std::unique_ptr<VideoSinkBroadcast>>;
    using Base = Bricks::LoggableS<CameraCapturerProxySink>;
public:
    CameraVideoSource(std::weak_ptr<rtc::Thread> signalingThread,
                      const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraVideoSource() override;
    const auto& signalingThread() const noexcept { return _observers.thread(); }
    void setDevice(MediaDevice device);
    void setCapability(webrtc::VideoCaptureCapability capability);
    bool enabled() const noexcept { return _enabled; }
    bool setEnabled(bool enabled);
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
    webrtc::MediaSourceInterface::SourceState state() const final { return _state; }
    bool remote() const final { return false; }
    // impl. of rtc::VideoSourceInterface<VideoFrame>
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) final;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
    // impl. of CameraObserver
    void onStateChanged(CameraState state) final;
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) final;
private:
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      std::string_view guid = {});
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      const rtc::scoped_refptr<CameraCapturer>& capturer);
    bool startCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer,
                       const webrtc::VideoCaptureCapability& capability) const;
    bool stopCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer) const;
    void logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                  const std::string& message, int code = 0) const;
    void logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer, const std::string& message) const;
    void requestCapturer();
    void resetCapturer();
    void destroyCapturer(); // non-threadsafe
    bool frameWanted() const;
    void changeState(webrtc::MediaSourceInterface::SourceState state);
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    AsyncListeners<webrtc::ObserverInterface*> _observers;
    Bricks::SafeObj<Broadcasters> _broadcasters;
    Bricks::SafeObj<MediaDevice> _device;
    SafeScopedRefPtr<CameraCapturer> _capturer;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _capability;
    std::atomic_bool _enabled = true;
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic<webrtc::MediaSourceInterface::SourceState> _state;
};

} // namespace LiveKitCpp
