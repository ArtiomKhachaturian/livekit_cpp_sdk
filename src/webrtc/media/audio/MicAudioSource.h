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
#pragma once // MicAudioSource.h
#include "AsyncMediaSource.h"
#include "AudioSourceImpl.h"
#include <api/media_stream_interface.h>
#include <rtc_base/weak_ptr.h>

namespace LiveKitCpp
{

class AdmProxy;

class MicAudioSource : public AsyncMediaSource<webrtc::AudioSourceInterface, AudioSourceImpl>
{
    using Base = AsyncMediaSource<webrtc::AudioSourceInterface, AudioSourceImpl>;
public:
    MicAudioSource(rtc::WeakPtr<AdmProxy> admProxyRef,
                   cricket::AudioOptions options = {},
                   std::weak_ptr<webrtc::TaskQueueBase> signalingQueue = {},
                   const std::shared_ptr<Bricks::Logger>& logger = {});
    MicAudioSource(const webrtc::scoped_refptr<AdmProxy>& adm,
                   cricket::AudioOptions options = {},
                   std::weak_ptr<webrtc::TaskQueueBase> signalingQueue = {});
    ~MicAudioSource() override;
    // impl. of webrtc::AudioSourceInterface
    void SetVolume(double volume) final;
    void RegisterAudioObserver(AudioObserver* observer) final;
    void UnregisterAudioObserver(AudioObserver* observer) final;
    void AddSink(webrtc::AudioTrackSinkInterface*  sink) final;
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) final;
    const cricket::AudioOptions options() const final;
private:
    const rtc::WeakPtr<AdmProxy> _admProxyRef;
    const cricket::AudioOptions _options;
};

} // namespace LiveKitCpp
