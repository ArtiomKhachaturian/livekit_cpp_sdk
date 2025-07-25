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
#pragma once // AdmProxyTransport.h
#include "Listeners.h"
#include <api/audio/audio_device_defines.h>

namespace webrtc {
class AudioTrackSinkInterface;
}

namespace LiveKitCpp
{

class AdmProxyTransport : public webrtc::AudioTransport
{
    using SinkRender = void(webrtc::AudioTrackSinkInterface::*)(const void* /*audio_data*/,
                                                                int /*bits_per_sample*/,
                                                                int /*sample_rate*/,
                                                                size_t /*number_of_channels*/,
                                                                size_t /*number_of_frames*/,
                                                                std::optional<int64_t> /* absolute_capture_timestamp_ms */);
public:
    AdmProxyTransport() = default;
    void setTargetTransport(webrtc::AudioTransport* transport);
    void addSink(webrtc::AudioTrackSinkInterface* sink);
    void removeSink(webrtc::AudioTrackSinkInterface* sink);
    void close();
    // impl. of webrtc::AudioTransport
    int32_t RecordedDataIsAvailable(const void* audioSamples,
                                    size_t nSamples,
                                    size_t nBytesPerSample,
                                    size_t nChannels,
                                    uint32_t samplesPerSec,
                                    uint32_t totalDelayMS,
                                    int32_t clockDrift,
                                    uint32_t currentMicLevel,
                                    bool keyPressed,
                                    uint32_t& newMicLevel) final;
    int32_t RecordedDataIsAvailable(const void* audioSamples,
                                    size_t nSamples,
                                    size_t nBytesPerSample,
                                    size_t nChannels,
                                    uint32_t samplesPerSec,
                                    uint32_t totalDelayMS,
                                    int32_t clockDrift,
                                    uint32_t currentMicLevel,
                                    bool keyPressed,
                                    uint32_t& newMicLevel,
                                    std::optional<int64_t> estimatedCaptureTimeNS) final;
    int32_t NeedMorePlayData(size_t nSamples, size_t nBytesPerSample,
                             size_t nChannels, uint32_t samplesPerSec,
                             void* audioSamples, size_t& nSamplesOut,  // NOLINT
                             int64_t* elapsedTimeMs, int64_t* ntpTimeMs) final;
    void PullRenderData(int bitsPerSample, int sampleRate, size_t numberOfChannels,
                        size_t numberOfFrames, void* audioData,
                        int64_t* elapsedTimeMs, int64_t* ntpTimeMs) final;
private:
    Bricks::SafeObj<webrtc::AudioTransport*> _targetTransport = nullptr;
    Bricks::Listeners<webrtc::AudioTrackSinkInterface*> _sinks;
};

} // namespace LiveKitCpp
