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
#pragma once // CameraVideoTrack.h
#include "Loggable.h"
#include "CameraVideoSource.h"
#include "MediaDevice.h"
#include <api/media_stream_interface.h>
#include <memory>

namespace LiveKitCpp
{

class CameraVideoTrack : public webrtc::VideoTrackInterface,
                         private Bricks::LoggableS<webrtc::ObserverInterface>
{
public:
    CameraVideoTrack(const std::string& id,
                     webrtc::scoped_refptr<CameraVideoSource> source,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraVideoTrack() override;
    void setDevice(MediaDevice device = {});
    void setCapability(webrtc::VideoCaptureCapability capability);
    // impl. of webrtc::VideoTrackInterface
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) final;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
    webrtc::VideoTrackSourceInterface* GetSource() const final;
    // impl. of MediaStreamTrackInterface
    std::string kind() const final { return webrtc::VideoTrackInterface::kVideoKind; }
    std::string id() const final { return _id; }
    bool enabled() const final;
    bool set_enabled(bool enable) final;
    webrtc::MediaStreamTrackInterface::TrackState state() const final { return _state; }
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
private:
    void changeState(webrtc::MediaStreamTrackInterface::TrackState state);
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
    // impl. of webrtc::ObserverInterface
    void OnChanged() final;
private:
    const std::string _id;
    const webrtc::scoped_refptr<CameraVideoSource> _source;
    AsyncListeners<webrtc::ObserverInterface*> _observers;
    std::atomic<webrtc::MediaStreamTrackInterface::TrackState> _state;
};

} // namespace LiveKitCpp
