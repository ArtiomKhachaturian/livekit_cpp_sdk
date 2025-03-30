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
#include "Utils.h"

namespace LiveKitCpp
{

AsyncMicSourceImpl::AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                       const std::shared_ptr<Bricks::Logger>& logger,
                                       std::weak_ptr<AdmProxyFacade> admProxy)
    : AsyncAudioSourceImpl(std::move(signalingQueue), logger, false)
    , _admProxy(std::move(admProxy))
{
    if (const auto admp = adm()) {
        const auto& state = admp->recordingState();
        admp->registerRecordingListener(this, true);
        _minVolume = state.minVolume();
        _maxVolume = state.maxVolume();
        _volume = state.volume();
    }
}

AsyncMicSourceImpl::~AsyncMicSourceImpl()
{
    if (const auto admp = adm()) {
        admp->registerRecordingListener(this, false);
    }
}

void AsyncMicSourceImpl::setVolume(double /*volume*/)
{
}

void AsyncMicSourceImpl::addSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (dynamic_cast<AudioSinks*>(sink)) {
        AsyncAudioSourceImpl::addSink(sink);
    }
}

void AsyncMicSourceImpl::removeSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (dynamic_cast<AudioSinks*>(sink)) {
        AsyncAudioSourceImpl::removeSink(sink);
    }
}

cricket::AudioOptions AsyncMicSourceImpl::options() const
{
    if (const auto admp = adm()) {
        return admp->options();
    }
    return {};
}

void AsyncMicSourceImpl::handleVolumeChanges() const
{
    const auto mmv = minMaxVolume();
    if (mmv.second - mmv.first) {
        const auto v = bound(mmv.first, _volume.load(), mmv.second);
        const double volume = (v - mmv.first) / double(mmv.second - mmv.first);
        AsyncAudioSourceImpl::onVolumeChanged(volume);
    }
}

std::pair<uint32_t, uint32_t> AsyncMicSourceImpl::minMaxVolume() const
{
    const auto minV = _minVolume.load();
    const auto maxV = _maxVolume.load();
    return {std::min(minV, maxV), std::max(minV, maxV)};
}

void AsyncMicSourceImpl::onMinMaxVolumeChanged(bool, uint32_t minVolume, uint32_t maxVolume)
{
    const auto b1 = exchangeVal(minVolume, _minVolume);
    const auto b2 = exchangeVal(maxVolume, _maxVolume);
    if (b1 || b2) {
        handleVolumeChanges();
    }
}

void AsyncMicSourceImpl::onVolumeChanged(bool, uint32_t volume)
{
    if (exchangeVal(volume, _volume)) {
        handleVolumeChanges();
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
}

void AsyncMicSourceImpl::onStopped(bool)
{
    changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
}

void AsyncMicSourceImpl::onMuteChanged(bool, bool mute)
{
    if (mute) {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
    else if (const auto admp = adm()) {
        if (admp->recordingState().started()) {
            changeState(webrtc::MediaSourceInterface::SourceState::kLive);
        }
        else {
            changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
        }
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
    }
}

} // namespace LiveKitCpp
