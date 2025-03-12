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
#include "RemoteTrack.h"
#include "TrackManager.h"

namespace LiveKitCpp
{

RemoteTrack::RemoteTrack(TrackManager* manager,
                         rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
    : _manager(manager)
    , _receiver(std::move(receiver))
    , _sid(_receiver ? _receiver->id() : std::string())
{
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RemoteTrack::raw() const
{
    if (_receiver) {
        return _receiver->track();
    }
    return {};
}

cricket::MediaType RemoteTrack::mediaType() const
{
    if (_receiver) {
        return _receiver->media_type();
    }
    return cricket::MEDIA_TYPE_UNSUPPORTED;
}

std::string RemoteTrack::sid() const
{
    return _sid;
}

void RemoteTrack::mute(bool mute)
{
    if (const auto track = raw()) {
        const auto wasEnabled = track->enabled();
        if (wasEnabled == mute) {
            track->set_enabled(!mute);
            if (_manager) {
                _manager->notifyAboutMuteChanges(_sid, mute);
            }
        }
    }
}

bool RemoteTrack::muted() const
{
    if (const auto track = raw()) {
        return !track->enabled();
    }
    return true;
}

} // namespace LiveKitCpp
