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
#pragma once // LocalParticipant.h
#include "AesCgmCryptorObserver.h"
#include "Listener.h"
#include "LocalAudioTrackImpl.h"
#include "LocalVideoTrackImpl.h"
#include "Loggable.h"
#include "ParticipantAccessor.h"
#include "SafeObjAliases.h"
#include "livekit/rtc/Participant.h"
#include "livekit/rtc/ParticipantListener.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"
#include <api/media_types.h>
#include <atomic>
#include <optional>
#include <vector>

namespace webrtc {
class AudioTrackInterface;
class RtpSenderInterface;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AudioDevice;
class LocalVideoDevice;
class LocalWebRtcTrack;
class TrackManager;
class ParticipantListener;
class PeerConnectionFactory;
class VideoDevice;
struct TrackPublishedResponse;
struct TrackUnpublishedResponse;
enum class EncryptionType;

class LocalParticipant : public Bricks::LoggableS<Participant, AesCgmCryptorObserver, ParticipantAccessor>
{
    using Base = Bricks::LoggableS<Participant, AesCgmCryptorObserver, ParticipantAccessor>;
    template <class T> using Tracks = Bricks::SafeObj<std::vector<std::shared_ptr<T>>>;
public:
    LocalParticipant(PeerConnectionFactory* pcf,
                     const Participant* session,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipant() final { reset(); }
    void reset();
    std::optional<bool> stereoRecording() const;
    size_t audioTracksCount() const;
    size_t videoTracksCount() const;
    std::shared_ptr<LocalAudioTrackImpl> addAudioTrack(std::shared_ptr<AudioDevice> device,
                                                       EncryptionType encryption,
                                                       const std::weak_ptr<TrackManager>& trackManager);
    std::shared_ptr<LocalVideoTrackImpl> addVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                       EncryptionType encryption,
                                                       const std::weak_ptr<TrackManager>& trackManager);
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeAudioTrack(std::shared_ptr<LocalAudioTrack> track);
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeVideoTrack(std::shared_ptr<LocalVideoTrack> track);
    std::shared_ptr<LocalAudioTrack> audioTrack(size_t index) const;
    std::shared_ptr<LocalVideoTrack> videoTrack(size_t index) const;
    std::vector<std::shared_ptr<LocalTrackAccessor>> tracks() const;
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> media() const;
    std::shared_ptr<LocalTrackAccessor> track(const std::string& id, bool cid,
                                      const std::optional<webrtc::MediaType>& hint = {}) const;
    std::shared_ptr<LocalTrackAccessor> track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const;
    void setListener(ParticipantListener* listener) { _listener = listener; }
    void setInfo(const ParticipantInfo& info);
    // impl. of Participant
    std::string sid() const final { return _sid(); }
    std::string identity() const final { return _identity(); }
    std::string name() const final { return _name(); }
    std::string metadata() const final { return _metadata(); }
    ParticipantKind kind() const final { return _kind; }
    // impl. of ParticipantImpl
    bool setRemoteSideTrackMute(const std::string& trackSid, bool mute) final;
    void setSpeakerChanges(float level, bool active) const final;
    void setConnectionQuality(ConnectionQuality quality, float score) final;
private:
    template <class TTracks>
    static std::shared_ptr<LocalTrackAccessor> lookup(const std::string& id, bool cid,
                                                      const TTracks& tracks);
    template <class TTrack, class TTracks>
    static void addTrack(const std::shared_ptr<TTrack>& track, TTracks& tracks);
    template <class TTracks>
    static void clear(TTracks& tracks);
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    // impl. of AesCgmCryptorObserver
    void onEncryptionStateChanged(webrtc::MediaType mediaType, const std::string&,
                                  const std::string& trackId, AesCgmCryptorState state) final;
private:
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    Bricks::SafeObj<const Participant*> _session;
    Bricks::Listener<ParticipantListener*> _listener;
    Tracks<LocalAudioTrackImpl> _audioTracks;
    Tracks<LocalVideoTrackImpl> _videoTracks;
    Bricks::SafeObj<std::string> _sid;
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _name;
    Bricks::SafeObj<std::string> _metadata;
    std::atomic<ParticipantKind> _kind = ParticipantKind::Standard;
};

} // namespace LiveKitCpp
