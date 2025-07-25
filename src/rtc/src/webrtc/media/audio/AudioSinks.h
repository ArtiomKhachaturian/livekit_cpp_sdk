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
#pragma once // AudioSinks.h
#include "Sinks.h"
#include "livekit/rtc/media/AudioSink.h"
#include <api/media_stream_interface.h>

namespace LiveKitCpp
{

class AudioSinks : public Sinks<AudioSink, webrtc::AudioTrackSinkInterface>
{
public:
    // impl. of webrtc::AudioTrackSinkInterface
    void OnData(const void* audioData, int bitsPerSample, int sampleRate,
                size_t numberOfChannels, size_t numberOfFrames,
                std::optional<int64_t> absoluteCaptureTimestampMs) final;
};

} // namespace LiveKitCpp
