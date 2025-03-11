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
#pragma once // LocalAudioTrackPromise.h
#include "LocalTrackImpl.h"

namespace LiveKitCpp
{
class LocalAudioTrack : public LocalTrackImpl<webrtc::AudioTrackInterface>
{
    using Base = LocalTrackImpl<webrtc::AudioTrackInterface>;
public:
    LocalAudioTrack(LocalTrackManager* manager, bool microphone = true,
                    const cricket::AudioOptions& options = {});
    // impl. of LocalTrack
    cricket::MediaType mediaType() const noexcept { return cricket::MEDIA_TYPE_AUDIO; }
    bool fillRequest(AddTrackRequest& request) const final;
protected:
    // impl. of LocalTrackPromise<>
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createMediaTrack(const std::string& id) final;
private:
    const bool _microphone;
    const cricket::AudioOptions _options;
};

} // namespace LiveKitCpp
