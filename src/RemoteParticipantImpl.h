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
#ifdef WEBRTC_AVAILABLE
#include "ParticipantImpl.h"
#include "RemoteParticipant.h"
#include "RemoteParticipantListener.h"
#include "SafeObj.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <optional>
#include <vector>

namespace webrtc {
class MediaStreamTrackInterface;
}

namespace LiveKitCpp
{

class RemoteAudioTrackImpl;
class RemoteVideoTrackImpl;
class TrackManager;

class RemoteParticipantImpl : public ParticipantImpl<RemoteParticipantListener,
                                                     RemoteParticipant>
{
    using Base = ParticipantImpl<RemoteParticipantListener, RemoteParticipant>;
    using AudioTracks = std::vector<std::shared_ptr<RemoteAudioTrackImpl>>;
    using VideoTracks = std::vector<std::shared_ptr<RemoteVideoTrackImpl>>;
public:
    RemoteParticipantImpl(const ParticipantInfo& info = {});
    ~RemoteParticipantImpl() final { reset(); }
    void reset();
    std::optional<TrackType> trackType(const std::string& sid) const;
    bool addAudio(const std::string& sid, TrackManager* manager,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    bool addVideo(const std::string& sid, TrackManager* manager,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    bool removeAudio(const std::string& sid);
    bool removeVideo(const std::string& sid);
    // overrides of ParticipantImpl<>
    void setInfo(const ParticipantInfo& info) final;
    // impl. of RemoteParticipant
    size_t audioTracksCount() const final;
    size_t videoTracksCount() const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(size_t index) const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(const std::string& sid) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(size_t index) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(const std::string& sid) const final;
private:
    const TrackInfo* findBySid(const std::string& sid) const;
    template<class TTrack>
    static std::optional<size_t> findBySid(const std::string& sid,
                                           const std::vector<TTrack>& collection);
    template<class TTrack>
    static bool removeTrack(const std::string& sid, std::vector<TTrack>& collection);
private:
    Bricks::SafeObj<AudioTracks> _audioTracks;
    Bricks::SafeObj<VideoTracks> _videoTracks;
};

} // namespace LiveKitCpp
#endif
