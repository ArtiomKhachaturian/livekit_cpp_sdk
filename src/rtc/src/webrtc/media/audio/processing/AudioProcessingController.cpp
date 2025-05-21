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
#include "AudioProcessingController.h"
#include "Listener.h"
#include "livekit/rtc/media/AudioSink.h"
#include <api/audio/audio_processing.h>
#include <common_audio/include/audio_util.h>

namespace LiveKitCpp
{

class AudioProcessingController::FramesWriter
{
public:
    void setRecWriter(AudioSink* writer) { _recWriter = writer; }
    void setPlayWriter(AudioSink* writer) { _playWriter = writer; }
    void writeRecAudioFrame(const int16_t* data, const webrtc::StreamConfig& config) const;
    void writePlayAudioFrame(const int16_t* data, const webrtc::StreamConfig& config) const;
private:
    Bricks::Listener<AudioSink*> _recWriter;
    Bricks::Listener<AudioSink*> _playWriter;
};

AudioProcessingController::AudioProcessingController()
    : _enableRecProcessing(std::make_shared<Flag>(true))
    , _enablePlayProcessing(std::make_shared<Flag>(true))
    , _writer(std::make_shared<FramesWriter>())
{
}

void AudioProcessingController::setEnableRecProcessing(bool enable)
{
    if (_enableRecProcessing) {
        _enableRecProcessing->store(enable);
    }
}

void AudioProcessingController::setEnablePlayProcessing(bool enable)
{
    if (_enablePlayProcessing) {
        _enablePlayProcessing->store(enable);
    }
}

bool AudioProcessingController::recProcessingEnabled() const
{
    return _enableRecProcessing && _enableRecProcessing->load();
}

bool AudioProcessingController::playProcessingEnabled() const
{
    return _enablePlayProcessing && _enablePlayProcessing->load();
}

void AudioProcessingController::setRecWriter(AudioSink* writer)
{
    if (_writer) {
        _writer->setRecWriter(writer);
    }
}

void AudioProcessingController::setPlayWriter(AudioSink* writer)
{
    if (_writer) {
        _writer->setPlayWriter(writer);
    }
}

void AudioProcessingController::commitRecAudioFrame(const int16_t* data,
                                                    const webrtc::StreamConfig& config) const
{
    if (_writer) {
        _writer->writeRecAudioFrame(data, config);
    }
}

void AudioProcessingController::commitRecAudioFrame(const float* data,
                                                    const webrtc::StreamConfig& config) const
{
    if (data) {
        static thread_local std::array<int16_t, webrtc::kMaxSamplesPerChannel10ms> int16Buffer;
        webrtc::FloatToS16(data, config.num_samples(), int16Buffer.data());
        commitRecAudioFrame(int16Buffer.data(), config);
    }
}

void AudioProcessingController::commitPlayAudioFrame(const int16_t* data,
                                                     const webrtc::StreamConfig& config) const
{
    if (_writer) {
        _writer->writePlayAudioFrame(data, config);
    }
}

void AudioProcessingController::commitPlayAudioFrame(const float* data,
                                                     const webrtc::StreamConfig& config) const
{
    if (data) {
        static thread_local std::array<int16_t, webrtc::kMaxSamplesPerChannel10ms> int16Buffer;
        webrtc::FloatToS16(data, config.num_samples(), int16Buffer.data());
        commitPlayAudioFrame(int16Buffer.data(), config);
    }
}

void AudioProcessingController::FramesWriter::writeRecAudioFrame(const int16_t* data,
                                                                 const webrtc::StreamConfig& config) const
{
    if (data) {
        if (const auto frames = config.num_frames()) {
            _recWriter.invoke(&AudioSink::onData, data, 16, config.sample_rate_hz(),
                              config.num_channels(), frames, std::nullopt);
        }
    }
}

void AudioProcessingController::FramesWriter::writePlayAudioFrame(const int16_t* data,
                                                                  const webrtc::StreamConfig& config) const
{
    if (data) {
        if (const auto frames = config.num_frames()) {
            _playWriter.invoke(&AudioSink::onData, data, 16, config.sample_rate_hz(),
                               config.num_channels(), frames, std::nullopt);
        }
    }
}

} // namespace LiveKitCpp
