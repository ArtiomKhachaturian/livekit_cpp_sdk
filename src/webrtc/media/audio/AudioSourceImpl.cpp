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
#include "AudioSourceImpl.h"

namespace LiveKitCpp
{

AudioSourceImpl::AudioSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                 const std::shared_ptr<Bricks::Logger>& logger,
                                 bool liveImmediately)
    : AsyncMediaSourceImpl(signalingQueue, logger, liveImmediately)
    , _observers(std::move(signalingQueue))
{
}

void AudioSourceImpl::setVolume(double volume)
{
    changeVolume(volume);
}

void AudioSourceImpl::registerAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer)
{
    _observers.add(observer);
}

void AudioSourceImpl::UnregisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer)
{
    _observers.remove(observer);
}

void AudioSourceImpl::addSink(webrtc::AudioTrackSinkInterface* sink)
{
    _sinks.add(sink);
}

void AudioSourceImpl::removeSink(webrtc::AudioTrackSinkInterface* sink)
{
    _sinks.remove(sink);
}

void AudioSourceImpl::onEnabled(bool enabled)
{
    AsyncMediaSourceImpl::onEnabled(enabled);
    if (enabled) {
        changeState(webrtc::MediaSourceInterface::SourceState::kLive);
    }
    else {
        changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
    }
}

void AudioSourceImpl::changeVolume(double volume)
{
    _observers.invoke(&webrtc::AudioSourceInterface::AudioObserver::OnSetVolume, volume);
}

void AudioSourceImpl::sendData(const void* audioData,
                               int bitsPerSample, int sampleRate,
                               size_t numberOfChannels, size_t numberOfFrames) const
{
    if (audioData && bitsPerSample > 0 && sampleRate > 0 && numberOfChannels && numberOfFrames) {
        using methodType = void(webrtc::AudioTrackSinkInterface::*)(const void*, int, int,
                                                                    size_t, size_t);
        _sinks.invoke<methodType>(&webrtc::AudioTrackSinkInterface::OnData, audioData,
                      bitsPerSample, sampleRate, numberOfChannels, numberOfFrames);
    }
}

} // namespace LiveKitCpp
