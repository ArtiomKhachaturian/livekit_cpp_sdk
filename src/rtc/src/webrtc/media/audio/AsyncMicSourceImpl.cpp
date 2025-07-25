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
#include "AsyncMicSourceImpl.h"
#include "AdmProxyFacade.h"
#include "AudioSinks.h"

namespace LiveKitCpp
{

AsyncMicSourceImpl::AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                       const std::shared_ptr<Bricks::Logger>& logger,
                                       webrtc::AudioOptions options,
                                       std::weak_ptr<AdmProxyFacade> admProxy)
    : AsyncAudioSourceImpl(std::move(signalingQueue), logger, false)
    , _options(std::move(options))
    , _admProxy(std::move(admProxy))
{
    if (const auto admp = adm()) {
        const auto& state = admp->recordingState();
        admp->registerRecordingListener(this, true);
        _volumeControl->setRange(state.minVolume(), state.maxVolume());
        _volumeControl->setVolume(state.volume());
    }
}

AsyncMicSourceImpl::~AsyncMicSourceImpl()
{
    if (const auto admp = adm()) {
        admp->registerRecordingListener(this, false);
    }
}

void AsyncMicSourceImpl::addSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (const auto sinks = dynamic_cast<AudioSinks*>(sink)) {
        if (const auto admp = adm()) {
            admp->registerRecordingSink(sink, true);
        }
    }
}

void AsyncMicSourceImpl::removeSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (const auto sinks = dynamic_cast<AudioSinks*>(sink)) {
        if (const auto admp = adm()) {
            admp->registerRecordingSink(sink, false);
        }
    }
}

void AsyncMicSourceImpl::onMinMaxVolumeChanged(bool, uint32_t minVolume, uint32_t maxVolume)
{
    LOCK_WRITE_SAFE_OBJ(_volumeControl);
    _volumeControl->setRange(minVolume, maxVolume);
}

void AsyncMicSourceImpl::onVolumeChanged(bool, uint32_t volume)
{
    std::optional<double> normalized;
    {
        LOCK_WRITE_SAFE_OBJ(_volumeControl);
        if (_volumeControl->setVolume(volume)) {
            normalized = _volumeControl->normalizedVolume();
        }
    }
    if (normalized) {
        AsyncAudioSourceImpl::onVolumeChanged(normalized.value());
    }
}

void AsyncMicSourceImpl::onStarted(bool)
{
    if (enabled()) {
        changeState(webrtc::MediaSourceInterface::SourceState::kLive);
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
    notify(&MediaDeviceListener::onMediaStarted);
}

void AsyncMicSourceImpl::onStopped(bool)
{
    if (enabled()) {
        changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
    notify(&MediaDeviceListener::onMediaStopped);
}

void AsyncMicSourceImpl::onMuteChanged(bool, bool mute)
{
    webrtc::MediaSourceInterface::SourceState newState;
    if (mute) {
        newState = webrtc::MediaSourceInterface::SourceState::kMuted;
    }
    else if (const auto admp = adm()) {
        if (enabled()) {
            if (admp->recordingState().started()) {
                newState = webrtc::MediaSourceInterface::SourceState::kLive;
            }
            else {
                newState = webrtc::MediaSourceInterface::SourceState::kInitializing;
            }
        }
        else {
            newState = webrtc::MediaSourceInterface::SourceState::kMuted;
        }
    }
    else {
        newState = webrtc::MediaSourceInterface::SourceState::kEnded;
    }
    changeState(newState);
}

void AsyncMicSourceImpl::onDeviceChanged(bool, const MediaDeviceInfo&)
{
    notify(&MediaDeviceListener::onMediaChanged);
}

} // namespace LiveKitCpp
