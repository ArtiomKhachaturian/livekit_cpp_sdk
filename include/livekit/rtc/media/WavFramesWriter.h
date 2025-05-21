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
#pragma once // WavFramesWriter.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/media/AudioSink.h"
#include <memory>

namespace LiveKitCpp
{

class LIVEKIT_RTC_API WavFramesWriter : public AudioSink
{
    struct Impl;
public:
    WavFramesWriter(std::string filename);
    WavFramesWriter(const WavFramesWriter&) = delete;
    WavFramesWriter(WavFramesWriter&&) noexcept = delete;
    ~WavFramesWriter() final;
    WavFramesWriter& operator = (const WavFramesWriter&) = delete;
    WavFramesWriter& operator = (WavFramesWriter&&) noexcept = delete;
    // impl. of AudioProcessingFramesWriter
    void onData(const int16_t* audioData, int bitsPerSample,
                int sampleRate, size_t numberOfChannels,
                size_t numberOfFrames,
                const std::optional<int64_t>& absoluteCaptureTimestampMs) final;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
