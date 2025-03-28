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
#pragma once // LocalAudioTrack.h
#include "LocalAudioSource.h"
#include <api/media_stream_interface.h>

namespace LiveKitCpp
{

class LocalAudioTrack : public webrtc::AudioTrackInterface
{
public:
    LocalAudioTrack(const std::string& id,
                    std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                    cricket::AudioOptions options = {},
                    const std::shared_ptr<Bricks::Logger>& logger = {});
    // impl. of webrtc::AudioTrackInterface
    webrtc::AudioSourceInterface* GetSource() const final;
    void AddSink(webrtc::AudioTrackSinkInterface* sink) final;
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) final;
    bool GetSignalLevel(int* level) final;
    rtc::scoped_refptr<webrtc::AudioProcessorInterface> GetAudioProcessor() final;
    // impl. of MediaStreamTrackInterface
    std::string kind() const final { return webrtc::AudioTrackInterface::kAudioKind; }
    std::string id() const final { return _id; }
    bool enabled() const final;
    bool set_enabled(bool enable) final;
    webrtc::MediaStreamTrackInterface::TrackState state() const final;
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
private:
    const std::string _id;
    const webrtc::scoped_refptr<LocalAudioSource> _source;
};

} // namespace LiveKitCpp
