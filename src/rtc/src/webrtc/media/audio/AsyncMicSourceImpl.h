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
#pragma once // AsyncMicSourceImpl.h
#include "AsyncAudioSourceImpl.h"
#include "AdmProxyListener.h"
#include "SafeObj.h"
#include "VolumeControl.h"
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class AdmProxyFacade;

class AsyncMicSourceImpl : public AsyncAudioSourceImpl, private AdmProxyListener
{
public:
    AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                       const std::shared_ptr<Bricks::Logger>& logger,
                       webrtc::AudioOptions options,
                       std::weak_ptr<AdmProxyFacade> admProxy);
    ~AsyncMicSourceImpl() override;
    // impl. of AsyncAudioSourceImpl
    void setVolume(double) final {}
    void addSink(webrtc::AudioTrackSinkInterface* sink) final;
    void removeSink(webrtc::AudioTrackSinkInterface* sink) final;
    webrtc::AudioOptions options() const final { return _options; }
private:
    std::shared_ptr<AdmProxyFacade> adm() const noexcept { return _admProxy.lock(); }
    // impl. of AdmProxyListener
    void onMinMaxVolumeChanged(bool, uint32_t minVolume, uint32_t maxVolume) final;
    void onVolumeChanged(bool, uint32_t volume) final;
    void onStarted(bool) final;
    void onStopped(bool) final;
    void onMuteChanged(bool, bool mute) final;
    void onDeviceChanged(bool, const MediaDeviceInfo&) final;
private:
    const webrtc::AudioOptions _options;
    const std::weak_ptr<AdmProxyFacade> _admProxy;
    Bricks::SafeObj<VolumeControl> _volumeControl;
};

} // namespace LiveKitCpp
