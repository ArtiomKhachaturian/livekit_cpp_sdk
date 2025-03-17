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
#include "LocalAudioTrackImpl.h"
#include "MediaAuthorization.h"

namespace {

// see enum class TrackSource
inline std::string audioLabel(bool microphone) {
    return microphone ? "microphone" : "screen_share_audio";
}

}
namespace LiveKitCpp
{

LocalAudioTrackImpl::LocalAudioTrackImpl(LocalTrackManager* manager, bool microphone,
                                         const std::shared_ptr<Bricks::Logger>& logger)
    : Base(audioLabel(microphone), manager, logger)
    , _microphone(microphone)
{
}

LocalAudioTrackImpl::~LocalAudioTrackImpl()
{
    installSink(false, audioSink());
}

TrackSource LocalAudioTrackImpl::source() const
{
    return _microphone ? TrackSource::Microphone : TrackSource::ScreenShareAudio;
}

void LocalAudioTrackImpl::fillRequest(AddTrackRequest* request) const
{
    Base::fillRequest(request);
    if (request) {
        request->_type = type();
        request->_source = source();
    }
}

void LocalAudioTrackImpl::requestAuthorization()
{
    Base::requestAuthorization();
    if (_microphone) {
        MediaAuthorization::query(MediaAuthorizationKind::Microphone, true, logger());
    }
}

void LocalAudioTrackImpl::installSink(bool install, webrtc::AudioTrackSinkInterface* sink)
{
    if (sink) {
        installSink(install, sink, mediaTrack());
    }
}

bool LocalAudioTrackImpl::signalLevel(int& level) const
{
    if (const auto track = mediaTrack()) {
        return track->GetSignalLevel(&level);
    }
    return false;
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalAudioTrackImpl::
    createMediaTrack(const std::string& id)
{
    if (const auto m = manager()) {
        auto track = m->createMic(id);
        if (track) {
            installSink(true, audioSink(), track);
        }
        return track;
    }
    return {};
}

void LocalAudioTrackImpl::installSink(bool install, webrtc::AudioTrackSinkInterface* sink,
                                      const webrtc::scoped_refptr<webrtc::AudioTrackInterface>& track)
{
    if (sink && track) {
        if (install) {
            track->AddSink(sink);
        }
        else {
            track->RemoveSink(sink);
        }
    }
}

} // namespace LiveKitCpp
