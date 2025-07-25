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
#include "AdmProxyTransport.h"
#include <api/media_stream_interface.h>

namespace LiveKitCpp
{

void AdmProxyTransport::setTargetTransport(webrtc::AudioTransport* transport)
{
    if (this != transport) {
        _targetTransport(transport);
    }
}

void AdmProxyTransport::addSink(webrtc::AudioTrackSinkInterface* sink)
{
    _sinks.add(sink);
}

void AdmProxyTransport::removeSink(webrtc::AudioTrackSinkInterface* sink)
{
    _sinks.remove(sink);
}

void AdmProxyTransport::close()
{
    _targetTransport(nullptr);
    _sinks.clear();
}

int32_t AdmProxyTransport::RecordedDataIsAvailable(const void* audioSamples,
                                                   size_t nSamples,
                                                   size_t nBytesPerSample,
                                                   size_t nChannels,
                                                   uint32_t samplesPerSec,
                                                   uint32_t totalDelayMS,
                                                   int32_t clockDrift,
                                                   uint32_t currentMicLevel,
                                                   bool keyPressed,
                                                   uint32_t& newMicLevel)
{
    return RecordedDataIsAvailable(audioSamples, nSamples, nBytesPerSample,
                                   nChannels, samplesPerSec,
                                   totalDelayMS, clockDrift, currentMicLevel,
                                   keyPressed, newMicLevel,
                                   /*capture_timestamp_ns=*/std::nullopt);
}

int32_t AdmProxyTransport::RecordedDataIsAvailable(const void* audioSamples,
                                                   size_t nSamples,
                                                   size_t nBytesPerSample,
                                                   size_t nChannels,
                                                   uint32_t samplesPerSec,
                                                   uint32_t totalDelayMS,
                                                   int32_t clockDrift,
                                                   uint32_t currentMicLevel,
                                                   bool keyPressed,
                                                   uint32_t& newMicLevel,
                                                   std::optional<int64_t> estimatedCaptureTimeNS)
{
    int32_t res = -1;
    LOCK_READ_SAFE_OBJ(_targetTransport);
    if (const auto transport = _targetTransport.constRef()) {
        res = transport->RecordedDataIsAvailable(audioSamples, nSamples,
                                                 nBytesPerSample, nChannels,
                                                 samplesPerSec, totalDelayMS,
                                                 clockDrift, currentMicLevel,
                                                 keyPressed, newMicLevel,
                                                 estimatedCaptureTimeNS);
    }
    if (0 == res) {
        std::optional<int64_t> absoluteCaptureTimestampMs;
        if (estimatedCaptureTimeNS.has_value()) {
            absoluteCaptureTimestampMs = estimatedCaptureTimeNS.value() / 1000000U;
        }
        _sinks.invoke<SinkRender>(&webrtc::AudioTrackSinkInterface::OnData,
                                  audioSamples, int(nBytesPerSample * 8U),
                                  int(samplesPerSec), nChannels, nSamples,
                                  absoluteCaptureTimestampMs);
    }
    return res;
}

int32_t AdmProxyTransport::NeedMorePlayData(size_t nSamples,
                                            size_t nBytesPerSample,
                                            size_t nChannels,
                                            uint32_t samplesPerSec,
                                            void* audioSamples,
                                            size_t& nSamplesOut,
                                            int64_t* elapsedTimeMs,
                                            int64_t* ntpTimeMs)
{
    LOCK_READ_SAFE_OBJ(_targetTransport);
    if (const auto transport = _targetTransport.constRef()) {
        return transport->NeedMorePlayData(nSamples, nBytesPerSample,
                                           nChannels, samplesPerSec,
                                           audioSamples, nSamplesOut,
                                           elapsedTimeMs, ntpTimeMs);
    }
    return -1;
}

void AdmProxyTransport::PullRenderData(int bitsPerSample, int sampleRate,
                                       size_t numberOfChannels,
                                       size_t numberOfFrames, void* audioData,
                                       int64_t* elapsedTimeMs, int64_t* ntpTimeMs)
{
    LOCK_READ_SAFE_OBJ(_targetTransport);
    if (const auto transport = _targetTransport.constRef()) {
        transport->PullRenderData(bitsPerSample, sampleRate,
                                  numberOfChannels, numberOfFrames,
                                  audioData, elapsedTimeMs, ntpTimeMs);
    }
}


} // namespace LiveKitCpp
