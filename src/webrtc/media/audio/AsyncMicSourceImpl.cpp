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

class AsyncMicSourceImpl::VolumeControl
{
public:
    VolumeControl() = default;
    void reset() { _minVolume = _maxVolume = 0U; }
    bool setRange(uint32_t minVolume, uint32_t maxVolume);
    bool valid() const;
    bool setNormalizedVolume(double volume);
    bool setVolume(uint32_t volume);
    uint32_t volume() const;
    double normalizedVolume() const noexcept { return _normalizedVolume; }
private:
    std::atomic<uint32_t> _minVolume = 0U;
    std::atomic<uint32_t> _maxVolume = 0U;
    std::atomic<double> _normalizedVolume = 0.;
};

AsyncMicSourceImpl::AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                       const std::shared_ptr<Bricks::Logger>& logger,
                                       std::weak_ptr<AdmProxyFacade> admProxy)
    : AsyncAudioSourceImpl(std::move(signalingQueue), logger, false)
    , _admProxy(std::move(admProxy))
    , _volumeControl(std::make_unique<VolumeControl>())
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

void AsyncMicSourceImpl::setVolume(double volume)
{
    if (_volumeControl->setNormalizedVolume(volume)) {
        requestVolumeChanges(_volumeControl->normalizedVolume());
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

cricket::AudioOptions AsyncMicSourceImpl::options() const
{
    if (const auto admp = adm()) {
        return admp->options();
    }
    return {};
}

void AsyncMicSourceImpl::onEnabled(bool enabled)
{
    AsyncAudioSourceImpl::onEnabled(enabled);
    if (enabled) {
        requestVolumeChanges(_volumeControl->normalizedVolume());
    }
}

void AsyncMicSourceImpl::requestVolumeChanges(double volume)
{
    if (enabled() && _volumeControl->valid()) {
        if (const auto admp = adm()) {
        }
    }
}

void AsyncMicSourceImpl::onMinMaxVolumeChanged(bool, uint32_t minVolume, uint32_t maxVolume)
{
    _volumeControl->setRange(minVolume, maxVolume);
}

void AsyncMicSourceImpl::onVolumeChanged(bool, uint32_t volume)
{
    if (_volumeControl->setVolume(volume)) {
        AsyncAudioSourceImpl::onVolumeChanged(_volumeControl->normalizedVolume());
    }
}

void AsyncMicSourceImpl::onStarted(bool)
{
    if (enabled()) {
        changeState(webrtc::MediaSourceInterface::SourceState::kLive);
        requestVolumeChanges(_volumeControl->normalizedVolume());
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
}

void AsyncMicSourceImpl::onStopped(bool)
{
    if (enabled()) {
        changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
}

void AsyncMicSourceImpl::onMuteChanged(bool, bool mute)
{
    webrtc::MediaSourceInterface::SourceState newState = state();
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

bool AsyncMicSourceImpl::VolumeControl::setRange(uint32_t minVolume, uint32_t maxVolume)
{
    const auto minv = std::min(minVolume, maxVolume);
    const auto maxv = std::max(minVolume, maxVolume);
    const auto b1 = exchangeVal(minv, _minVolume);
    const auto b2 = exchangeVal(maxv, _maxVolume);
    return b1 || b2;
}

bool AsyncMicSourceImpl::VolumeControl::valid() const
{
    return (_maxVolume - _minVolume) > 0U;
}

bool AsyncMicSourceImpl::VolumeControl::setNormalizedVolume(double volume)
{
    return exchangeVal(bound(0., volume, 1.), _normalizedVolume);
}

bool AsyncMicSourceImpl::VolumeControl::setVolume(uint32_t volume)
{
    const auto minv = _minVolume.load();
    const auto maxv = _maxVolume.load();
    if (const auto range = maxv - minv) {
        volume = bound(minv, volume, maxv);
        const double normv = (volume - minv) / double(range);
        return setNormalizedVolume(normv);
    }
    return false;
}

uint32_t AsyncMicSourceImpl::VolumeControl::volume() const
{
    const auto minv = _minVolume.load();
    if (const auto range = _maxVolume - minv) {
        return minv + uint32_t(std::round(normalizedVolume() * range));
    }
    return 0U;
}

} // namespace LiveKitCpp
