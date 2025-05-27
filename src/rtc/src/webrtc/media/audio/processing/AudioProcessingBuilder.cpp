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
#include "AudioProcessingBuilder.h"
#include "ControlledAudioProcessor.h"
#ifdef USE_RN_NOISE_SUPPRESSOR
#include "AudioProcessor.h"
#endif
#include "Utils.h"
#include <api/make_ref_counted.h>

namespace LiveKitCpp
{

AudioProcessingBuilder::AudioProcessingBuilder(const AudioProcessingController& controller)
    : _controller(controller)
{
}

AudioProcessingBuilder::~AudioProcessingBuilder() = default;

absl::Nullable<webrtc::scoped_refptr<webrtc::AudioProcessing>>
    AudioProcessingBuilder::Build(const webrtc::Environment& env)
{
    auto processing = _default.Build(env);
    if (processing) {
#ifdef USE_RN_NOISE_SUPPRESSOR
        processing = webrtc::make_ref_counted<AudioProcessor>(std::move(processing));
#endif
        return webrtc::make_ref_counted<ControlledAudioProcessor>(std::move(processing), _controller);
    }
    return {};
}

} // namespace LiveKitCpp
