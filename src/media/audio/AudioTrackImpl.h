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
#include "AudioTrackSinks.h"
#include "TrackImpl.h"
#include <type_traits>

namespace LiveKitCpp
{

template<class TTrackApi = AudioTrack>
class AudioTrackImpl : public TrackImpl<webrtc::AudioTrackInterface, TTrackApi>
{
    static_assert(std::is_base_of_v<AudioTrack, TTrackApi>);
    using Base = TrackImpl<webrtc::AudioTrackInterface, TTrackApi>;
public:
    ~AudioTrackImpl() override;
    // impl. of AudioTrack
    void addSink(AudioTrackSink* sink) final;
    void removeSink(AudioTrackSink* sink) final;
    std::optional<int> signalLevel() const final;
    void setVolume(double volume) final;
protected:
    AudioTrackImpl(webrtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack,
                   TrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger);
    webrtc::AudioSourceInterface* audioSource() const;
private:
    AudioTrackSinks _sinks;
};

template<class TTrackApi>
inline AudioTrackImpl<TTrackApi>::AudioTrackImpl(webrtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack,
                                                 TrackManager* manager,
                                                 const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(audioTrack), manager, logger)
{
}

template<class TTrackApi>
inline AudioTrackImpl<TTrackApi>::~AudioTrackImpl()
{
    if (_sinks.clear()) {
        if (const auto& t = Base::mediaTrack()) {
            t->RemoveSink(&_sinks);
        }
    }
}

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::addSink(AudioTrackSink* sink)
{
    if (Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        if (const auto& t = Base::mediaTrack()) {
            t->AddSink(&_sinks);
        }
    }
}

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::removeSink(AudioTrackSink* sink)
{
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink)) {
        if (const auto& t = Base::mediaTrack()) {
            t->RemoveSink(&_sinks);
        }
    }
}

template<class TTrackApi>
inline std::optional<int> AudioTrackImpl<TTrackApi>::signalLevel() const
{
    if (const auto& t = Base::mediaTrack()) {
        int level;
        if (t->GetSignalLevel(&level)) {
            return level;
        }
    }
    return std::nullopt;
}

template<class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::setVolume(double volume)
{
    if (const auto s = audioSource()) {
        s->SetVolume(volume);
    }
}

template<class TTrackApi>
webrtc::AudioSourceInterface* AudioTrackImpl<TTrackApi>::audioSource() const
{
    if (const auto& t = Base::mediaTrack()) {
        return t->GetSource();
    }
    return nullptr;
}

} // namespace LiveKitCpp
