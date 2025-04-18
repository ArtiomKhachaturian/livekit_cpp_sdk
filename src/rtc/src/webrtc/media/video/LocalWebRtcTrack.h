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
#pragma once // LocalWebRtcTrack.h
#include "AsyncListeners.h"
#include "AsyncVideoSource.h"
#include "Loggable.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <api/media_stream_interface.h>
#include <memory>

namespace LiveKitCpp
{

class MediaDeviceListener;

class LocalWebRtcTrack : public webrtc::VideoTrackInterface
{
public:
    LocalWebRtcTrack(const std::string& id, webrtc::scoped_refptr<AsyncVideoSource> source);
    ~LocalWebRtcTrack() override;
    void close();
    void setDeviceInfo(MediaDeviceInfo info);
    MediaDeviceInfo deviceInfo() const;
    void setOptions(VideoOptions options);
    VideoOptions options() const;
    void addListener(MediaDeviceListener* listener);
    void removeListener(MediaDeviceListener* listener);
    // impl. of webrtc::VideoTrackInterface
    webrtc::VideoTrackInterface::ContentHint content_hint() const final;
    void set_content_hint(webrtc::VideoTrackInterface::ContentHint hint) final;
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) final;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
    webrtc::VideoTrackSourceInterface* GetSource() const final;
    // impl. of MediaStreamTrackInterface
    std::string kind() const final { return webrtc::VideoTrackInterface::kVideoKind; }
    std::string id() const final { return _id; }
    bool enabled() const final;
    bool set_enabled(bool enable) final;
    webrtc::MediaStreamTrackInterface::TrackState state() const final;
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
private:
    const std::string _id;
    const webrtc::scoped_refptr<AsyncVideoSource> _source;
};

} // namespace LiveKitCpp
