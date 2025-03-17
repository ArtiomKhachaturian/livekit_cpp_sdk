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
#pragma once // AudioTrackImpl.h
#include "AudioTrack.h"
#include "AudioTrackSink.h"
#include "Listeners.h"
#include <api/media_stream_interface.h>
#include <type_traits>

namespace LiveKitCpp
{

template<class TTrackApi = AudioTrack>
class AudioTrackImpl : public TTrackApi,
                       private webrtc::AudioTrackSinkInterface
{
    static_assert(std::is_base_of_v<AudioTrack, TTrackApi>);
public:
    ~AudioTrackImpl() override { _sinks.clear(); }
    // impl. of AudioTrack
    void addSink(AudioTrackSink* sink) final;
    void removeSink(AudioTrackSink* sink) final;
    std::optional<int> signalLevel() const final;
protected:
    AudioTrackImpl() = default;
    webrtc::AudioTrackSinkInterface* audioSink();
    virtual void installSink(bool install, webrtc::AudioTrackSinkInterface* sink) = 0;
    virtual bool signalLevel(int& level) const = 0;
private:
    // impl. of webrtc::AudioTrackSinkInterface
    void OnData(const void* audioData, int bitsPerSample, int sampleRate,
                size_t numberOfChannels, size_t numberOfFrames,
                std::optional<int64_t> absoluteCaptureTimestampMs) final;
private:
    Bricks::Listeners<AudioTrackSink*> _sinks;
};

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::addSink(AudioTrackSink* sink)
{
    if (sink) {
        const auto res = _sinks.add(sink);
        if (Bricks::AddResult::OkFirst == res) {
            installSink(true, this);
        }
    }
}

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::removeSink(AudioTrackSink* sink)
{
    if (sink) {
        const auto res = _sinks.remove(sink);
        if (Bricks::RemoveResult::OkLast == res) {
            installSink(false, this);
        }
    }
}

template<class TTrackApi>
inline std::optional<int> AudioTrackImpl<TTrackApi>::signalLevel() const
{
    int level;
    if (signalLevel(level)) {
        return level;
    }
    return std::nullopt;
}

template<class TTrackApi>
inline webrtc::AudioTrackSinkInterface* AudioTrackImpl<TTrackApi>::audioSink()
{
    return _sinks.empty() ? nullptr : this;
}

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::OnData(const void* audioData,
                                              int bitsPerSample,
                                              int sampleRate,
                                              size_t numberOfChannels,
                                              size_t numberOfFrames,
                                              std::optional<int64_t> absoluteCaptureTimestampMs)
{
    if (audioData) {
        _sinks.invoke(&AudioTrackSink::onData, audioData, bitsPerSample,
                      sampleRate, numberOfChannels, numberOfFrames,
                      absoluteCaptureTimestampMs);
    }
}

} // namespace LiveKitCpp
