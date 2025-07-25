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
#pragma once // ControlledAudioProcessor.h
#include "AudioProcessingWrapper.h"
#include "AudioProcessingController.h"

namespace LiveKitCpp
{

class ControlledAudioProcessor : public AudioProcessingWrapper
{
public:
    ControlledAudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> impl,
                             const AudioProcessingController& controller);
    // overrides of AudioProcessingWrapper
    int ProcessStream(const int16_t* const src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      int16_t* const dest) final;
    int ProcessStream(const float* const* src,
                      const webrtc::StreamConfig& inputConfig,
                      const webrtc::StreamConfig& outputConfig,
                      float* const* dest) final;
    int ProcessReverseStream(const int16_t* const src,
                             const webrtc::StreamConfig& inputConfig,
                             const webrtc::StreamConfig& outputConfig,
                             int16_t* const dest) final;
    int AnalyzeReverseStream(const float* const* data,
                             const webrtc::StreamConfig& reverseConfig) final;
    int ProcessReverseStream(const float* const* src,
                             const webrtc::StreamConfig& inputConfig,
                             const webrtc::StreamConfig& outputConfig,
                             float* const* dest) final;
    webrtc::AudioProcessingStats GetStatistics(bool hasRemoteTracks) final;
    webrtc::AudioProcessingStats GetStatistics() final;
private:
    const AudioProcessingController _controller;
};

} // namespace LiveKitCpp
