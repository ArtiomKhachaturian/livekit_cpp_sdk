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
#pragma once // RemoteParticipantImpl.h
#include "ParticipantImpl.h"
#include "RemoteParticipant.h"
#include "SafeObj.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <vector>

namespace webrtc {
class MediaStreamTrackInterface;
}

namespace LiveKitCpp
{

class RemoteAudioTrackImpl;
class RemoteVideoTrackImpl;
class TrackManager;

class RemoteParticipantImpl : public ParticipantImpl<RemoteParticipant>
{
    using AudioTracks = std::vector<std::shared_ptr<RemoteAudioTrackImpl>>;
    using VideoTracks = std::vector<std::shared_ptr<RemoteVideoTrackImpl>>;
public:
    RemoteParticipantImpl(const ParticipantInfo& info = {});
    bool addAudio(const std::string& sid, TrackManager* manager,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    bool addVideo(const std::string& sid, TrackManager* manager,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    bool removeMedia(const std::string& sid);
    size_t audioTracksCount() const final;
    size_t videoTracksCount() const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(size_t index) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(size_t index) const final;
private:
    const TrackInfo* findBySid(const std::string& sid) const;
    template<class TCollection>
    static bool removeTrack(const std::string& sid, TCollection& collection);
private:
    Bricks::SafeObj<AudioTracks> _audioTracks;
    Bricks::SafeObj<VideoTracks> _videoTracks;
};

} // namespace LiveKitCpp
