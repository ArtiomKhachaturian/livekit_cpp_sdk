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
#pragma once // AudioProcessor.h
#ifdef USE_RN_NOISE_SUPPRESSOR
#include "AudioProcessingWrapper.h"
#include "SafeObjAliases.h"

namespace webrtc {
class AudioBuffer;
}

namespace LiveKitCpp
{

class RnNoiseAudioProcessor : public AudioProcessingWrapper
{
    class Denoiser;
public:
    RnNoiseAudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> standardProcessing);
    ~RnNoiseAudioProcessor() override;
    // overrides of AudioProcessingWrapper
    void ApplyConfig(const webrtc::AudioProcessing::Config& config) final;
    int ProcessStream(const int16_t* const src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      int16_t* const dest) final;
    int ProcessStream(const float* const* src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      float* const* dest) final;
private:
    void createDenoiser();
    void destroyDenoiser();
private:
    Bricks::SafeUniquePtr<Denoiser> _denoiser;
};

} // namespace LiveKitCpp
#endif
