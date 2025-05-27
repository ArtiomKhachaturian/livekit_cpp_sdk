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
#include "Listener.h"
#include "Loggable.h"
#include "ParticipantAccessor.h"
#include "SafeObj.h"
#include "livekit/rtc/RemoteParticipant.h"
#include "livekit/rtc/RemoteParticipantListener.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <memory>
#include <optional>
#include <vector>

namespace webrtc {
class RtpReceiverInterface;
}

namespace LiveKitCpp
{

class RemoteAudioTrackImpl;
class RemoteVideoTrackImpl;
class RtpReceiversStorage;
class TrackManager;

class RemoteParticipantImpl : public Bricks::LoggableS<RemoteParticipant, ParticipantAccessor>
{
    template <class T> using Tracks = std::vector<std::shared_ptr<T>>;
    using AudioTracks = Tracks<RemoteAudioTrackImpl>;
    using VideoTracks = Tracks<RemoteVideoTrackImpl>;
    class ListenerImpl;
public:
    RemoteParticipantImpl(const std::shared_ptr<RtpReceiversStorage>& receiversStorage,
                          const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RemoteParticipantImpl() final { reset(); }
    void reset();
    std::optional<TrackType> trackType(const std::string& trackSid) const;
    bool addAudio(const std::string& trackSid,
                  const std::weak_ptr<TrackManager>& trackManager,
                  webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver = {});
    bool addVideo(const std::string& trackSid,
                  const std::weak_ptr<TrackManager>& trackManager,
                  webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver = {});
    bool removeAudio(const std::string& trackSid);
    bool removeVideo(const std::string& trackSid);
    ParticipantInfo info() const { return _info(); }
    void setInfo(const std::weak_ptr<TrackManager>& trackManager, const ParticipantInfo& info);
    bool setRemoteSideTrackMute(const std::string& trackSid, bool mute);
    // impl. of Participant
    std::string sid() const final;
    std::string identity() const final;
    std::string name() const final;
    std::string metadata() const final;
    ParticipantKind kind() const final;
    // impl. of RemoteParticipant
    void addListener(RemoteParticipantListener* listener) final;
    void removeListener(RemoteParticipantListener* listener) final;
    bool hasActivePublisher() const final;
    ParticipantState state() const final;
    size_t audioTracksCount() const final;
    size_t videoTracksCount() const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(size_t index) const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(const std::string& sid) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(size_t index) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(const std::string& sid) const final;
    // impl. of ParticipantImpl
    void setSpeakerChanges(float level, bool active) const final;
    void setConnectionQuality(ConnectionQuality quality, float score) final;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    bool updateAudio(const TrackInfo& trackInfo) const;
    bool updateVideo(const TrackInfo& trackInfo) const;
    const TrackInfo* findBySid(const std::string& trackSid) const;
    TrackInfo* findBySid(const std::string& trackSid);
    template <class TTrack>
    bool addTrack(const std::string& trackSid,
                  const std::weak_ptr<TrackManager>& trackManager,
                  webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                  Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    template <class TTrack>
    bool removeTrack(const std::string& trackSid, Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    template <class TTrack>
    void clearTracks(Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    template <class TTrack>
    static std::optional<size_t> findBySid(const std::string& trackSid,
                                           const Tracks<TTrack>& collection);
private:
    const std::shared_ptr<RtpReceiversStorage> _receiversStorage;
    const std::shared_ptr<ListenerImpl> _listener;
    Bricks::SafeObj<ParticipantInfo> _info;
    Bricks::SafeObj<AudioTracks> _audioTracks;
    Bricks::SafeObj<VideoTracks> _videoTracks;
};

} // namespace LiveKitCpp
