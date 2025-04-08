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
#ifdef WEBRTC_AVAILABLE
#include "AudioDeviceImpl.h"
#include "TrackImpl.h"
#include "media/AudioTrack.h"
#include <type_traits>

namespace LiveKitCpp
{

template <class TTrackApi = AudioTrack>
class AudioTrackImpl : public TrackImpl<AudioDeviceImpl, TTrackApi>
{
    static_assert(std::is_base_of_v<AudioTrack, TTrackApi>);
    using Base = TrackImpl<AudioDeviceImpl, TTrackApi>;
public:
    ~AudioTrackImpl() override = default;
    // impl. of AudioTrack
    void addSink(AudioSink* sink) final;
    void removeSink(AudioSink* sink) final;
    std::optional<int> signalLevel() const final;
    void setVolume(double volume) final;
protected:
    AudioTrackImpl(std::shared_ptr<AudioDeviceImpl> audioDevice, TrackManager* manager);
    webrtc::AudioSourceInterface* audioSource() const;
};

template <class TTrackApi>
inline AudioTrackImpl<TTrackApi>::AudioTrackImpl(std::shared_ptr<AudioDeviceImpl> audioDevice,
                                                 TrackManager* manager)
    : Base(std::move(audioDevice), manager)
{
}

template <class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::addSink(AudioSink* sink)
{
    if (const auto& md = Base::mediaDevice()) {
        md->addSink(sink);
    }
}

template <class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::removeSink(AudioSink* sink)
{
    if (const auto& md = Base::mediaDevice()) {
        md->removeSink(sink);
    }
}

template <class TTrackApi>
inline std::optional<int> AudioTrackImpl<TTrackApi>::signalLevel() const
{
    if (const auto& md = Base::mediaDevice()) {
        return md->signalLevel();
    }
    return std::nullopt;
}

template <class TTrackApi>
inline void AudioTrackImpl<TTrackApi>::setVolume(double volume)
{
    if (const auto& md = Base::mediaDevice()) {
        md->setVolume(volume);
    }
}

template <class TTrackApi>
webrtc::AudioSourceInterface* AudioTrackImpl<TTrackApi>::audioSource() const
{
    if (const auto& md = Base::mediaDevice()) {
        if (const auto& track = md->track()) {
            return track->GetSource();
        }
    }
    return nullptr;
}

} // namespace LiveKitCpp
#endif
