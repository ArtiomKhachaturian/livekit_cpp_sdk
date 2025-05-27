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
#include <modules/audio_processing/audio_buffer.h>
#include <rtc_base/checks.h>

namespace
{

// Options for gracefully handling processing errors.
enum class FormatErrorOutputOption
{
  kOutputExactCopyOfInput,
  kOutputBroadcastCopyOfFirstInputChannel,
  kOutputSilence,
  kDoNothing
};

enum class AudioFormatValidity
{
  // Format is supported by APM.
  kValidAndSupported,
  // Format has a reasonable interpretation but is not supported.
  kValidButUnsupportedSampleRate,
  // The remaining enums values signal that the audio does not have a reasonable
  // interpretation and cannot be used.
  kInvalidSampleRate,
  kInvalidChannelCount
};

AudioFormatValidity validateAudioFormat(const webrtc::StreamConfig& config);
webrtc::AudioProcessing::Error audioFormatValidityToErrorCode(AudioFormatValidity validity);
// Returns an AudioProcessing::Error together with the best possible option for
// output audio content.
std::pair<webrtc::AudioProcessing::Error, FormatErrorOutputOption>
    chooseErrorOutputOption(const webrtc::StreamConfig& inputConfig,
                            const webrtc::StreamConfig& outputConfig);
// Checks if the audio format is supported. If not, the output is populated in a
// best-effort manner and an APM error code is returned.
webrtc::AudioProcessing::Error handleUnsupportedAudioFormats(const int16_t* const src,
                                                             const webrtc::StreamConfig& inputConfig,
                                                             const webrtc::StreamConfig& outputConfig,
                                                             int16_t* const dest);
// Checks if the audio format is supported. If not, the output is populated in a
// best-effort manner and an APM error code is returned.
webrtc::AudioProcessing::Error handleUnsupportedAudioFormats(const float* const* src,
                                                             const webrtc::StreamConfig& inputConfig,
                                                             const webrtc::StreamConfig& outputConfig,
                                                             float* const* dest);
}

#define SLIDING_WINDOW_SIZE (FRAME_SIZE / 8)

namespace LiveKitCpp
{

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
    if (src && hasDenoiser()) {
        static thread_local std::array<float, webrtc::kMaxSamplesPerChannel10ms> floatBuffer;
        webrtc::S16ToFloatS16(src, inputConfig.num_frames(), floatBuffer.data());
        denoise(floatBuffer.data(), inputConfig);
        webrtc::FloatS16ToS16(floatBuffer.data(), inputConfig.num_frames(), const_cast<int16_t*>(src));
    }
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

int AudioProcessor::ProcessStream(const float* const* src,
                                  const webrtc::StreamConfig& inputConfig,
                                  const webrtc::StreamConfig& outputConfig,
                                  float* const* dest)
{
    if (src) {
        denoise(*src, inputConfig);
    }
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

void AudioProcessor::denoise(const float* data, const webrtc::StreamConfig& config) const
{
    if (auto writable = const_cast<float*>(data)) {
        LOCK_READ_SAFE_OBJ(_denoiser);
        if (const auto denoiser = _denoiser.constRef()) {
            if (1U == config.num_channels()) {
                static thread_local std::array<float, webrtc::kMaxSamplesPerChannel10ms> output;
                rnnoise_process_frame(denoiser, output.data(), data);
                std::memcpy(writable, output.data(), output.size());
            }
        }
    }
}

bool AudioProcessor::hasDenoiser() const
{
    LOCK_READ_SAFE_OBJ(_denoiser);
    return nullptr != _denoiser;
}

void AudioProcessor::createDenoiser()
{
    LOCK_WRITE_SAFE_OBJ(_denoiser);
    if (!_denoiser.constRef()) {
        _denoiser = rnnoise_create(nullptr);
    }
}

void AudioProcessor::destroyDenoiser()
{
    LOCK_WRITE_SAFE_OBJ(_denoiser);
    if (const auto denoiser = _denoiser.exchange(nullptr)) {
        rnnoise_destroy(denoiser);
    }
}

} // namespace LiveKitCpp

namespace
{

AudioFormatValidity ValidateAudioFormat(const webrtc::StreamConfig& config)
{
    if (config.sample_rate_hz() < 0) {
        return AudioFormatValidity::kInvalidSampleRate;
    }
    if (config.num_channels() == 0) {
        return AudioFormatValidity::kInvalidChannelCount;
    }
    // Format has a reasonable interpretation, but may still be unsupported.
    if (config.sample_rate_hz() < 8000 || config.sample_rate_hz() > webrtc::AudioBuffer::kMaxSampleRate) {
        return AudioFormatValidity::kValidButUnsupportedSampleRate;
    }
    // Format is fully supported.
    return AudioFormatValidity::kValidAndSupported;
}

webrtc::AudioProcessing::Error audioFormatValidityToErrorCode(AudioFormatValidity validity)
{
    switch (validity) {
        case AudioFormatValidity::kValidAndSupported:
            return webrtc::AudioProcessing::kNoError;
        case AudioFormatValidity::kValidButUnsupportedSampleRate:  // fall-through
        case AudioFormatValidity::kInvalidSampleRate:
            return webrtc::AudioProcessing::kBadSampleRateError;
        case AudioFormatValidity::kInvalidChannelCount:
            return webrtc::AudioProcessing::kBadNumberChannelsError;
    }
    RTC_DCHECK(false);
}

std::pair<webrtc::AudioProcessing::Error, FormatErrorOutputOption>
    chooseErrorOutputOption(const webrtc::StreamConfig& inputConfig,
                            const webrtc::StreamConfig& outputConfig)
{
    AudioFormatValidity inputValidity = validateAudioFormat(inputConfig);
    AudioFormatValidity outputValidity = validateAudioFormat(outputConfig);

    if (inputValidity == AudioFormatValidity::kValidAndSupported &&
        outputValidity == AudioFormatValidity::kValidAndSupported &&
        (outputConfig.num_channels() == 1 ||
         outputConfig.num_channels() == inputConfig.num_channels())) {
        return std::make_pair(webrtc::AudioProcessing::kNoError, FormatErrorOutputOption::kDoNothing);
    }

    webrtc::AudioProcessing::Error errorCode = audioFormatValidityToErrorCode(inputValidity);
    if (errorCode == webrtc::AudioProcessing::kNoError) {
        errorCode = audioFormatValidityToErrorCode(outputValidity);
    }
    if (errorCode == webrtc::AudioProcessing::kNoError) {
        // The individual formats are valid but there is some error - must be
        // channel mismatch.
        errorCode = webrtc::AudioProcessing::kBadNumberChannelsError;
    }

    FormatErrorOutputOption outputOption;
    if (outputValidity != AudioFormatValidity::kValidAndSupported &&
        outputValidity != AudioFormatValidity::kValidButUnsupportedSampleRate) {
        // The output format is uninterpretable: cannot do anything.
        outputOption = FormatErrorOutputOption::kDoNothing;
    } else if (inputValidity != AudioFormatValidity::kValidAndSupported &&
               inputValidity != AudioFormatValidity::kValidButUnsupportedSampleRate) {
        // The input format is uninterpretable: cannot use it, must output silence.
        outputOption = FormatErrorOutputOption::kOutputSilence;
    } else if (inputConfig.sample_rate_hz() != outputConfig.sample_rate_hz()) {
        // Sample rates do not match: Cannot copy input into output, output silence.
        // Note: If the sample rates are in a supported range, we could resample.
        // However, that would significantly increase complexity of this error
        // handling code.
        outputOption = FormatErrorOutputOption::kOutputSilence;
    } else if (inputConfig.num_channels() != outputConfig.num_channels()) {
        // Channel counts do not match: We cannot easily map input channels to
        // output channels.
        outputOption = FormatErrorOutputOption::kOutputBroadcastCopyOfFirstInputChannel;
    } else {
        // The formats match exactly.
        RTC_DCHECK(inputConfig == outputConfig);
        outputOption = FormatErrorOutputOption::kOutputExactCopyOfInput;
    }
    return std::make_pair(errorCode, outputOption);
}

webrtc::AudioProcessing::Error handleUnsupportedAudioFormats(const int16_t* const src,
                                                             const webrtc::StreamConfig& inputConfig,
                                                             const webrtc::StreamConfig& outputConfig,
                                                             int16_t* const dest)
{
    RTC_DCHECK(src);
    RTC_DCHECK(dest);
    auto [errorCode, outputOption] = chooseErrorOutputOption(inputConfig, outputConfig);
    if (errorCode == webrtc::AudioProcessing::kNoError) {
        return webrtc::AudioProcessing::kNoError;
    }
    const size_t numOutputChannels = outputConfig.num_channels();
    switch (outputOption) {
        case FormatErrorOutputOption::kOutputSilence:
            std::memset(dest, 0, outputConfig.num_samples() * sizeof(int16_t));
            break;
        case FormatErrorOutputOption::kOutputBroadcastCopyOfFirstInputChannel:
            for (size_t i = 0; i < outputConfig.num_frames(); ++i) {
                int16_t sample = src[inputConfig.num_channels() * i];
                for (size_t ch = 0; ch < numOutputChannels; ++ch) {
                    dest[ch + numOutputChannels * i] = sample;
                }
            }
            break;
        case FormatErrorOutputOption::kOutputExactCopyOfInput:
            std::memcpy(dest, src, outputConfig.num_samples() * sizeof(int16_t));
            break;
        case FormatErrorOutputOption::kDoNothing:
            break;
    }
    return errorCode;
}

webrtc::AudioProcessing::Error handleUnsupportedAudioFormats(const float* const* src,
                                                             const webrtc::StreamConfig& inputConfig,
                                                             const webrtc::StreamConfig& outputConfig,
                                                             float* const* dest)
{
    RTC_DCHECK(src);
    RTC_DCHECK(dest);
    for (size_t i = 0; i < inputConfig.num_channels(); ++i) {
        RTC_DCHECK(src[i]);
    }
    for (size_t i = 0; i < outputConfig.num_channels(); ++i) {
        RTC_DCHECK(dest[i]);
    }
    auto [errorCode, outputOption] = chooseErrorOutputOption(inputConfig, outputConfig);
    if (errorCode == webrtc::AudioProcessing::kNoError) {
        return webrtc::AudioProcessing::kNoError;
    }
    const size_t numOutputChannels = outputConfig.num_channels();
    switch (outputOption) {
        case FormatErrorOutputOption::kOutputSilence:
            for (size_t ch = 0; ch < numOutputChannels; ++ch) {
                std::memset(dest[ch], 0, outputConfig.num_frames() * sizeof(float));
            }
            break;
        case FormatErrorOutputOption::kOutputBroadcastCopyOfFirstInputChannel:
            for (size_t ch = 0; ch < numOutputChannels; ++ch) {
                std::memcpy(dest[ch], src[0], outputConfig.num_frames() * sizeof(float));
            }
            break;
        case FormatErrorOutputOption::kOutputExactCopyOfInput:
            for (size_t ch = 0; ch < numOutputChannels; ++ch) {
                std::memcpy(dest[ch], src[ch], outputConfig.num_frames() * sizeof(float));
            }
            break;
        case FormatErrorOutputOption::kDoNothing:
            break;
    }
    return errorCode;
}

}

#endif
