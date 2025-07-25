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
#include "ControlledAudioProcessor.h"

namespace LiveKitCpp
{

ControlledAudioProcessor::ControlledAudioProcessor(webrtc::scoped_refptr<webrtc::AudioProcessing> impl,
                                                   const AudioProcessingController& controller)
    : AudioProcessingWrapper(std::move(impl))
    , _controller(controller)
{
}

int ControlledAudioProcessor::ProcessStream(const int16_t* const src,
                                            const webrtc::StreamConfig& inputConfig,
                                            const webrtc::StreamConfig& outputConfig,
                                            int16_t* const dest)
{
    int result = kNoError;
    if (_controller.recProcessingEnabled()) {
        result = AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
    }
    if (kNoError == result) {
        _controller.commitRecAudioFrame(dest, outputConfig);
    }
    return result;
}

int ControlledAudioProcessor::ProcessStream(const float* const* src,
                                            const webrtc::StreamConfig& inputConfig,
                                            const webrtc::StreamConfig& outputConfig,
                                            float* const* dest)
{
    int result = kNoError;
    if (_controller.recProcessingEnabled()) {
        result = AudioProcessingWrapper::ProcessStream(src, inputConfig, outputConfig, dest);
    }
    if (kNoError == result && dest) {
        _controller.commitRecAudioFrame(*dest, outputConfig);
    }
    return result;
}

int ControlledAudioProcessor::ProcessReverseStream(const int16_t* const src,
                                                   const webrtc::StreamConfig& inputConfig,
                                                   const webrtc::StreamConfig& outputConfig,
                                                   int16_t* const dest)
{
    int result = kNoError;
    if (_controller.playProcessingEnabled()) {
        result = AudioProcessingWrapper::ProcessReverseStream(src, inputConfig, outputConfig, dest);
    }
    if (kNoError == result) {
        _controller.commitPlayAudioFrame(dest, outputConfig);
    }
    return result;
}

int ControlledAudioProcessor::AnalyzeReverseStream(const float* const* data,
                                                   const webrtc::StreamConfig& reverseConfig)
{
    int result = kNoError;
    if (_controller.playProcessingEnabled()) {
        result = AnalyzeReverseStream(data, reverseConfig);
    }
    return result;
}

int ControlledAudioProcessor::ProcessReverseStream(const float* const* src,
                                                   const webrtc::StreamConfig& inputConfig,
                                                   const webrtc::StreamConfig& outputConfig,
                                                   float* const* dest)
{
    int result = kNoError;
    if (_controller.playProcessingEnabled()) {
        result = AudioProcessingWrapper::ProcessReverseStream(src, inputConfig, outputConfig, dest);
    }
    if (kNoError == result && dest) {
        _controller.commitPlayAudioFrame(*dest, outputConfig);
    }
    return result;
}

webrtc::AudioProcessingStats ControlledAudioProcessor::GetStatistics(bool hasRemoteTracks)
{
    if (_controller.processingEnabled()) {
        return AudioProcessingWrapper::GetStatistics(hasRemoteTracks);
    }
    return {};
}

webrtc::AudioProcessingStats ControlledAudioProcessor::GetStatistics()
{
    if (_controller.processingEnabled()) {
        return AudioProcessingWrapper::GetStatistics();
    }
    return {};
}

} // namespace LiveKitCpp
