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
#pragma once // AudioSink.h
#include <cstdint>
#include <optional>

namespace LiveKitCpp
{

class AudioSink
{
public:
    // In this method, `absolute_capture_timestamp_ms`, when available, is
    // supposed to deliver the timestamp when this audio frame was originally
    // captured. This timestamp MUST be based on the same clock as
    // rtc::TimeMillis().
    // audio data format is Int16
    virtual void onData(const int16_t* audioData, int bitsPerSample,
                        int sampleRate, size_t numberOfChannels,
                        size_t numberOfFrames,
                        const std::optional<int64_t>& absoluteCaptureTimestampMs) = 0;
protected:
    virtual ~AudioSink() = default;
};

} // namespace LiveKitCpp
