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
#include "LocalTrackManager.h"
#include "MediaAuthorization.h"
#include <api/media_stream_interface.h>

namespace {

// see enum class TrackSource
inline std::string audioLabel(bool microphone) {
    return microphone ? "microphone" : "screen_share_audio";
}

}
namespace LiveKitCpp
{

LocalAudioTrack::LocalAudioTrack(LocalTrackManager* manager, bool microphone,
                                 const std::shared_ptr<Bricks::Logger>& logger)
    : LocalTrackImpl<webrtc::AudioTrackInterface>(audioLabel(microphone), manager, logger)
    , _microphone(microphone)
{
}

void LocalAudioTrack::fillRequest(AddTrackRequest* request) const
{
    LocalTrackImpl<webrtc::AudioTrackInterface>::fillRequest(request);
    if (request) {
        request->_source = _microphone ? TrackSource::Microphone : TrackSource::ScreenShareAudio;
    }
}

void LocalAudioTrack::requestAuthorization()
{
    LocalTrackImpl<webrtc::AudioTrackInterface>::requestAuthorization();
    if (_microphone) {
        MediaAuthorization::maybeAuthorized(MediaAuthorizationKind::Microphone, logger());
    }
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalAudioTrack::createMediaTrack(const std::string& id)
{
    if (const auto m = manager()) {
        return m->createAudio(id);
    }
    return {};
}

} // namespace LiveKitCpp
