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
#include "livekit/rtc/media/AudioFramesWriter.h"
#include <api/audio/audio_processing.h>
#include <common_audio/include/audio_util.h>

namespace LiveKitCpp
{

class AudioProcessingController::FramesWriter
{
public:
    void setRecWriter(AudioFramesWriter* writer) { _recWriter = writer; }
    void setPlayWriter(AudioFramesWriter* writer) { _playWriter = writer; }
    void writeRecAudioFrame(const int16_t* data, const webrtc::StreamConfig& config) const;
    void writePlayAudioFrame(const int16_t* data, const webrtc::StreamConfig& config) const;
    void notifyThatRecStarted(bool started);
    void notifyThatPlayStarted(bool started);
private:
    Bricks::Listener<AudioFramesWriter*> _recWriter;
    Bricks::Listener<AudioFramesWriter*> _playWriter;
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

void AudioProcessingController::setRecWriter(AudioFramesWriter* writer)
{
    if (_writer) {
        _writer->setRecWriter(writer);
    }
}

void AudioProcessingController::setPlayWriter(AudioFramesWriter* writer)
{
    if (_writer) {
        _writer->setPlayWriter(writer);
    }
}

void AudioProcessingController::notifyThatRecStarted(bool started)
{
    if (_writer) {
        _writer->notifyThatRecStarted(started);
    }
}

void AudioProcessingController::notifyThatPlayStarted(bool started)
{
    if (_writer) {
        _writer->notifyThatPlayStarted(started);
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
            _recWriter.invoke(&AudioFramesWriter::onData, data, 16, config.sample_rate_hz(),
                              config.num_channels(), frames, std::nullopt);
        }
    }
}

void AudioProcessingController::FramesWriter::writePlayAudioFrame(const int16_t* data,
                                                                  const webrtc::StreamConfig& config) const
{
    if (data) {
        if (const auto frames = config.num_frames()) {
            _playWriter.invoke(&AudioFramesWriter::onData, data, 16, config.sample_rate_hz(),
                               config.num_channels(), frames, std::nullopt);
        }
    }
}

void AudioProcessingController::FramesWriter::notifyThatRecStarted(bool started)
{
    if (started) {
        _recWriter.invoke(&AudioFramesWriter::onStarted);
    }
    else {
        _recWriter.invoke(&AudioFramesWriter::onStopped);
    }
}

void AudioProcessingController::FramesWriter::notifyThatPlayStarted(bool started)
{
    if (started) {
        _playWriter.invoke(&AudioFramesWriter::onStarted);
    }
    else {
        _playWriter.invoke(&AudioFramesWriter::onStopped);
    }
}

} // namespace LiveKitCpp
