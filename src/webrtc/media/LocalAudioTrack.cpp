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
#include "LocalAudioTrack.h"

namespace {

// see enum class TrackSource
inline std::string audioLabel(bool microphone) {
    return microphone ? "microphone" : "screen_share_audio";
}

}
namespace LiveKitCpp
{

LocalAudioTrack::LocalAudioTrack(LocalTrackManager* manager, bool microphone,
                                 const cricket::AudioOptions& options)
    : Base(audioLabel(microphone), manager)
    , _microphone(microphone)
    , _options(options)
{
}

bool LocalAudioTrack::fillRequest(AddTrackRequest& request) const
{
    if (Base::fillRequest(request)) {
        request._source = _microphone ? TrackSource::Microphone : TrackSource::ScreenShareAudio;
        return true;
    }
    return false;
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalAudioTrack::createMediaTrack(const std::string& id)
{
    if (const auto m = manager()) {
        return m->createAudio(id, _options);
    }
    return {};
}

} // namespace LiveKitCpp
