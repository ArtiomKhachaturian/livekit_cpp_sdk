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
extern "C" {
#include "rnnoise.h"
#include "denoise.h"
}
#include <modules/audio_processing/audio_buffer.h>

#define SLIDING_WINDOW_SIZE (FRAME_SIZE / 8)

namespace LiveKitCpp
{

AudioProcessor::AudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> impl)
    : AudioProcessingWrapper(std::move(impl))
{
}

int AudioProcessor::ProcessStream(const int16_t* const src,
                                  const webrtc::StreamConfig& inputConfig,
                                  const webrtc::StreamConfig& outputConfig,
                                  int16_t* const dest)
{
    applyDenoiser(src, inputConfig);
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

int AudioProcessor::ProcessStream(const float* const* src,
                                  const webrtc::StreamConfig& inputConfig,
                                  const webrtc::StreamConfig& outputConfig,
                                  float* const* dest)
{
    //applyDenoiser(src, inputConfig);
    return AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
}

template <typename T>
void AudioProcessor::applyDenoiser(const T* const data,
                                   const webrtc::StreamConfig& inputConfig)
{
    if (auto writable = const_cast<T*>(data)) {
        webrtc::AudioBuffer buffer(inputConfig.sample_rate_hz(), inputConfig.num_channels(),
                                   inputConfig.sample_rate_hz(), inputConfig.num_channels(),
                                   inputConfig.sample_rate_hz(), inputConfig.num_channels());
        buffer.CopyFrom(data, inputConfig);
        // Create RNNoise state
        DenoiseState* st = rnnoise_create(NULL);
        // Process each channel in the audio buffer
        for (size_t ch = 0; ch < inputConfig.num_channels(); ++ch) {
            // Create a temporary buffer to hold the current frame
            float x[FRAME_SIZE];
            float y[FRAME_SIZE];
            size_t num_frames = buffer.num_frames();
            size_t num_frames2 = inputConfig.num_frames();

            // Process each sliding window
            for (size_t window_start = 0; window_start < num_frames * FRAME_SIZE; window_start += SLIDING_WINDOW_SIZE) {
                // size_t window_end = std::min(window_start + FRAME_SIZE, num_frames * FRAME_SIZE);
                size_t frame_idx = window_start / FRAME_SIZE;

                // Copy the current frame to the input buffer for RNNoise
                for (size_t i = 0; i < FRAME_SIZE; ++i) {
                    size_t sample_idx = frame_idx * FRAME_SIZE + i;
                    if (sample_idx < num_frames * FRAME_SIZE) {
                        x[i] = buffer.split_bands(ch)[0][sample_idx];
                        //x[i] = 0.0f;
                    } else {
                        // Zero-pad if we reach the end of the audio buffer
                        x[i] = 0.0f;
                    }
                }

                // Process the frame with RNNoise
                rnnoise_process_frame(st, x, y);

                // Copy the denoised frame back to the audio buffer
                for (size_t i = 0; i < FRAME_SIZE; ++i) {
                    size_t sample_idx = frame_idx * FRAME_SIZE + i;
                    if (sample_idx < num_frames * FRAME_SIZE) {
                        buffer.split_bands(ch)[0][sample_idx] = y[i];
                    }
                }
            }
        }
        // Clean up RNNoise state
        rnnoise_destroy(st);
        //buffer.CopyTo(inputConfig, writable);
    }
}

} // namespace LiveKitCpp
#endif
