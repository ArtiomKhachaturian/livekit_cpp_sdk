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
#ifdef USE_RN_NOISE_SUPPRESSOR
#include "AudioProcessor.h"
#include "rnnoise.h"
#include <modules/audio_processing/audio_buffer.h>
#include <rtc_base/checks.h>

namespace LiveKitCpp
{

class AudioProcessor::RnDenoiser
{
public:
    RnDenoiser();
    ~RnDenoiser();
    void process(const int16_t* src, const webrtc::StreamConfig& config);
    void process(const float* const* src, const webrtc::StreamConfig& config);
private:
    void denoise(webrtc::AudioBuffer* buffer);
    void denoiseChannel(float* channel, size_t numFrames);
private:
    DenoiseState* const _state;
};

AudioProcessor::AudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> impl)
    : AudioProcessingWrapper(std::move(impl))
{
}

AudioProcessor::~AudioProcessor()
{
    destroyDenoiser();
}

void AudioProcessor::ApplyConfig(const webrtc::AudioProcessing::Config& config)
{
    AudioProcessingWrapper::ApplyConfig(config);
    if (config.noise_suppression.enabled) {
        createDenoiser();
    }
    else {
        destroyDenoiser();
    }
}

int AudioProcessor::ProcessStream(const int16_t* const src,
                                  const webrtc::StreamConfig& inputConfig,
                                  const webrtc::StreamConfig& outputConfig,
                                  int16_t* const dest)
{
    if (src) {
        LOCK_READ_SAFE_OBJ(_denoiser);
        if (const auto& denoiser = _denoiser.constRef()) {
            denoiser->process(src, inputConfig);
        }
    }
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

int AudioProcessor::ProcessStream(const float* const* src,
                                  const webrtc::StreamConfig& inputConfig,
                                  const webrtc::StreamConfig& outputConfig,
                                  float* const* dest)
{
    if (src) {
        LOCK_READ_SAFE_OBJ(_denoiser);
        if (const auto& denoiser = _denoiser.constRef()) {
            denoiser->process(src, inputConfig);
        }
    }
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

void AudioProcessor::createDenoiser()
{
    LOCK_WRITE_SAFE_OBJ(_denoiser);
    if (!_denoiser.constRef()) {
        _denoiser = std::make_unique<RnDenoiser>();
    }
}

void AudioProcessor::destroyDenoiser()
{
    _denoiser({});
}

AudioProcessor::RnDenoiser::RnDenoiser()
    : _state(rnnoise_create(nullptr))
{
}

AudioProcessor::RnDenoiser::~RnDenoiser()
{
    rnnoise_destroy(_state);
}

void AudioProcessor::RnDenoiser::process(const int16_t* src, const webrtc::StreamConfig& config)
{
    if (src) {
        webrtc::AudioBuffer buffer(config.sample_rate_hz(), config.num_channels(),
                                   config.sample_rate_hz(), config.num_channels(),
                                   config.sample_rate_hz(), config.num_channels());
        buffer.CopyFrom(src, config);
        denoise(&buffer);
        buffer.CopyTo(config, const_cast<int16_t*>(src));
    }
}

void AudioProcessor::RnDenoiser::process(const float* const* src, const webrtc::StreamConfig& config)
{
    if (src) {
        webrtc::AudioBuffer buffer(config.sample_rate_hz(), config.num_channels(),
                                   config.sample_rate_hz(), config.num_channels(),
                                   config.sample_rate_hz(), config.num_channels());
        buffer.CopyFrom(src, config);
        denoise(&buffer);
        buffer.CopyTo(config, const_cast<float**>(src));
    }
}

void AudioProcessor::RnDenoiser::denoise(webrtc::AudioBuffer* buffer)
{
    if (buffer) {
        const size_t numFrames = buffer->num_frames();
        for (size_t ch = 0U; ch < buffer->num_channels(); ++ch) {
            denoiseChannel(buffer->channels()[ch], numFrames);
        }
    }
}

void AudioProcessor::RnDenoiser::denoiseChannel(float* channel, size_t numFrames)
{
    if (channel && numFrames) {
        if (numFrames == rnnoise_get_frame_size()) {
            rnnoise_process_frame(_state, channel, channel);
        }
        else {
            // sliding window
            static thread_local std::vector<float> processingBuffer;
            processingBuffer.resize(rnnoise_get_frame_size());
            // not yet implemented
            RTC_DCHECK_NOTREACHED();
        }
    }
}

} // namespace LiveKitCpp
#endif
