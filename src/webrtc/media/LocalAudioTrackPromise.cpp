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
#include "LocalAudioTrackPromise.h"

namespace LiveKitCpp
{

LocalAudioTrackPromise::LocalAudioTrackPromise(std::string label, LocalTrackFactory* factory,
                                               const cricket::AudioOptions& options)
    : LocalTrackPromise<webrtc::AudioTrackInterface>(std::move(label), factory)
    , _options(options)
{
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalAudioTrackPromise::createMediaTrack()
{
    if (const auto f = factory()) {
        return f->createAudio(label(), _options);
    }
    return {};
}

} // namespace LiveKitCpp
