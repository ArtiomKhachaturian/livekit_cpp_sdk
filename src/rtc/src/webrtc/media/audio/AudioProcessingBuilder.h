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
#pragma once // AudioProcessingBuilder.h
#include "AudioProcessingController.h"
#include <api/audio/builtin_audio_processing_builder.h>
#include <rtc_base/weak_ptr.h>
#include <atomic>
#include <list>

namespace LiveKitCpp
{

class AudioProcessingWrapper;

class AudioProcessingBuilder : public webrtc::AudioProcessingBuilderInterface
{
    using ProcessingList = std::list<webrtc::scoped_refptr<AudioProcessingWrapper>>;
public:
    AudioProcessingBuilder(const AudioProcessingController& controller);
    ~AudioProcessingBuilder() override;
    // impl. of webrtc::AudioProcessingBuilderInterface
    absl::Nullable<webrtc::scoped_refptr<webrtc::AudioProcessing>> Build(const webrtc::Environment& env) final;
private:
    const AudioProcessingController _controller;
    webrtc::BuiltinAudioProcessingBuilder _default;
};

} // namespace LiveKitCpp
