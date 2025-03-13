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
#include "Loggable.h"
#include "CameraCapturerProxySink.h"
#include "CameraObserver.h"
#include "Listeners.h"
#include "SafeScopedRefPtr.h"


#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <api/media_stream_interface.h>
#include <modules/video_capture/video_capture_defines.h>
#include <media/base/adapted_video_track_source.h>
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <atomic>
#include <memory>
#include <optional>
#include <unordered_set>


namespace LiveKitCpp
{

class CameraCapturer;

class CameraVideoSource : public webrtc::VideoTrackSourceInterface,
                          private Bricks::LoggableS<CameraCapturerProxySink>
{
public:
    CameraVideoSource(const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraVideoSource() override;
    void setCapturer(rtc::scoped_refptr<CameraCapturer> capturer);
    void setCapability(webrtc::VideoCaptureCapability capability);
    void enableBlackFrames(bool enable);
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
    // impl. of CameraCapturerProxySink
    void onStateChanged(CameraState state) final;
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) final;
private:
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      std::string_view guid = {});
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      const rtc::scoped_refptr<CameraCapturer>& capturer);
    bool start(const rtc::scoped_refptr<CameraCapturer>& capturer,
               const webrtc::VideoCaptureCapability& capability) const;
    bool stop(const rtc::scoped_refptr<CameraCapturer>& capturer) const;
    bool applyRotation() const;
    void broadcast(const webrtc::VideoFrame& frame);
    void changeState(webrtc::MediaSourceInterface::SourceState state);
    // Reports the appropriate frame size after adaptation. Returns true
    // if a frame is wanted. Returns false if there are no interested
    // sinks, or if the VideoAdapter decides to drop the frame.
    bool adaptFrame(int width, int height, int64_t timeUs,
                    int& outWidth, int& outHeight,
                    int& cropWidth, int& cropHeight,
                    int& cropX, int& cropY);
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    cricket::VideoAdapter _adapter;
    rtc::VideoBroadcaster _broadcaster;
    Bricks::Listeners<webrtc::ObserverInterface*> _observers;
    SafeScopedRefPtr<CameraCapturer> _capturer;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _capability;
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic_bool _hasLastResolution = false;
    std::atomic<webrtc::MediaSourceInterface::SourceState> _state;
};

} // namespace LiveKitCpp
