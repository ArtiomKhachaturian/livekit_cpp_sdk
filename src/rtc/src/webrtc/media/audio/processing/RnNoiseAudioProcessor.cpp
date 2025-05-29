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
#include "RnNoiseAudioProcessor.h"
#include "rnnoise.h"
#include <modules/audio_processing/audio_buffer.h>
#include <rtc_base/checks.h>

namespace
{

inline webrtc::AudioBuffer makeBuffer(const webrtc::StreamConfig& config) {
    return webrtc::AudioBuffer(config.sample_rate_hz(), config.num_channels(),
                               config.sample_rate_hz(), config.num_channels(),
                               config.sample_rate_hz(), config.num_channels());
}

}

namespace LiveKitCpp
{

class RnNoiseAudioProcessor::Denoiser
{
public:
    Denoiser();
    ~Denoiser();
    void process(const int16_t* src, const webrtc::StreamConfig& config);
    void process(const float* const* src, const webrtc::StreamConfig& config);
private:
    void denoise(webrtc::AudioBuffer* buffer);
    void denoiseChannel(float* channel, size_t numFrames);
private:
    DenoiseState* const _state;
};

RnNoiseAudioProcessor::RnNoiseAudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> standardProcessing)
    : AudioProcessingWrapper(std::move(standardProcessing))
{
}

RnNoiseAudioProcessor::~RnNoiseAudioProcessor()
{
    destroyDenoiser();
}

void RnNoiseAudioProcessor::ApplyConfig(const webrtc::AudioProcessing::Config& config)
{
    AudioProcessingWrapper::ApplyConfig(config);
    if (config.noise_suppression.enabled) {
        createDenoiser();
    }
    else {
        destroyDenoiser();
    }
}

int RnNoiseAudioProcessor::ProcessStream(const int16_t* const src,
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

int RnNoiseAudioProcessor::ProcessStream(const float* const* src,
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

void RnNoiseAudioProcessor::createDenoiser()
{
    LOCK_WRITE_SAFE_OBJ(_denoiser);
    if (!_denoiser.constRef()) {
        _denoiser = std::make_unique<Denoiser>();
    }
}

void RnNoiseAudioProcessor::destroyDenoiser()
{
    _denoiser({});
}

RnNoiseAudioProcessor::Denoiser::Denoiser()
    : _state(rnnoise_create(nullptr))
{
}

RnNoiseAudioProcessor::Denoiser::~Denoiser()
{
    rnnoise_destroy(_state);
}

void RnNoiseAudioProcessor::Denoiser::process(const int16_t* src,
                                              const webrtc::StreamConfig& config)
{
    if (src) {
        auto buffer = makeBuffer(config);
        buffer.CopyFrom(src, config);
        denoise(&buffer);
        buffer.CopyTo(config, const_cast<int16_t*>(src));
    }
}

void RnNoiseAudioProcessor::Denoiser::process(const float* const* src,
                                              const webrtc::StreamConfig& config)
{
    if (src) {
        auto buffer = makeBuffer(config);
        buffer.CopyFrom(src, config);
        denoise(&buffer);
        buffer.CopyTo(config, const_cast<float**>(src));
    }
}

void RnNoiseAudioProcessor::Denoiser::denoise(webrtc::AudioBuffer* buffer)
{
    if (buffer) {
        const size_t numFrames = buffer->num_frames();
        for (size_t ch = 0U; ch < buffer->num_channels(); ++ch) {
            denoiseChannel(buffer->channels()[ch], numFrames);
        }
    }
}

void RnNoiseAudioProcessor::Denoiser::denoiseChannel(float* channel, size_t numFrames)
{
    if (channel && numFrames) {
        const auto maxFrames = rnnoise_get_frame_size();
        // sliding window
        for (size_t i = 0U; i < numFrames; i += maxFrames) {
            const auto data = channel + (sizeof(float) * i);
            if (numFrames - i < maxFrames) {
                const auto rest = numFrames - i;
                const auto size = sizeof(float) * rest;
                std::vector<float> processingBuffer;
                processingBuffer.resize(maxFrames);
                // copy source data to processing buffer
                std::memcpy(processingBuffer.data(), data, size);
                // padding zero
                std::memset(processingBuffer.data() + size, 0, processingBuffer.size() - size);
                rnnoise_process_frame(_state, processingBuffer.data(), processingBuffer.data());
                // copy processed data to source buffer
                std::memcpy(data, processingBuffer.data(), size);
            }
            else {
                rnnoise_process_frame(_state, data, data);
            }
        }
    }
}

} // namespace LiveKitCpp
#endif
