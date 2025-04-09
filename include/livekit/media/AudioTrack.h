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
#pragma once // AudioTrack.h
#include "livekit/media/Track.h"
#include "livekit/rtc/AudioTrackFeature.h"
#include <optional>
#include <vector>

namespace LiveKitCpp
{

class AudioSink;

class AudioTrack : public Track
{
public:
    // impl. of Track
    TrackType type() const final { return TrackType::Audio; }
    virtual void addSink(AudioSink* sink) = 0;
    virtual void removeSink(AudioSink* sink) = 0;
    // Sets the volume of the track. `volume` is in  the range of [0, 10].
    virtual void setVolume(double volume) = 0;
    // Get the signal level from the audio track.
    virtual std::optional<int> signalLevel() const = 0;
    virtual std::vector<AudioTrackFeature> features() const { return {}; }
};

} // namespace LiveKitCpp
