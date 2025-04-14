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
class E2ESecurityFactory;

class RemoteParticipantImpl : public RemoteParticipant
{
    template <class T> using Tracks = std::vector<std::shared_ptr<T>>;
    using AudioTracks = Tracks<RemoteAudioTrackImpl>;
    using VideoTracks = Tracks<RemoteVideoTrackImpl>;
    class ListenerImpl;
public:
    RemoteParticipantImpl(E2ESecurityFactory* securityFactory,
                          const std::shared_ptr<RtpReceiversStorage>& receiversStorage,
                          const ParticipantInfo& info = {});
    ~RemoteParticipantImpl() final { reset(); }
    void reset();
    bool setRemoteSideTrackMute(const std::string& sid, bool mute);
    std::optional<TrackType> trackType(const std::string& sid) const;
    bool addAudio(const std::string& sid);
    bool addAudio(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    bool addVideo(const std::string& sid);
    bool addVideo(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    bool removeAudio(const std::string& sid);
    bool removeVideo(const std::string& sid);
    ParticipantInfo info() const { return _info(); }
    void setInfo(const ParticipantInfo& info);
    // impl. of Participant
    std::string sid() const final;
    std::string identity() const final;
    std::string name() const final;
    std::string metadata() const final;
    ParticipantKind kind() const final;
    // impl. of RemoteParticipant
    void setListener(RemoteParticipantListener* listener) final;
    bool hasActivePublisher() const final;
    ParticipantState state() const final;
    size_t audioTracksCount() const final;
    size_t videoTracksCount() const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(size_t index) const final;
    std::shared_ptr<RemoteAudioTrack> audioTrack(const std::string& sid) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(size_t index) const final;
    std::shared_ptr<RemoteVideoTrack> videoTrack(const std::string& sid) const final;
private:
    bool updateAudio(const TrackInfo& trackInfo) const;
    bool updateVideo(const TrackInfo& trackInfo) const;
    const TrackInfo* findBySid(const std::string& sid) const;
    TrackInfo* findBySid(const std::string& sid);
    template <class TTrack>
    bool addTrack(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                  Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    template <class TTrack>
    bool removeTrack(const std::string& sid, Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    template <class TTrack>
    void clearTracks(Bricks::SafeObj<Tracks<TTrack>>& collection) const;
    bool attachCryptor(EncryptionType encryption,
                       const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) const;
    template <class TTrack>
    static std::optional<size_t> findBySid(const std::string& sid,
                                           const Tracks<TTrack>& collection);
private:
    E2ESecurityFactory* const _securityFactory;
    const std::shared_ptr<RtpReceiversStorage> _receiversStorage;
    const std::shared_ptr<ListenerImpl> _listener;
    Bricks::SafeObj<ParticipantInfo> _info;
    Bricks::SafeObj<AudioTracks> _audioTracks;
    Bricks::SafeObj<VideoTracks> _videoTracks;
};

} // namespace LiveKitCpp
