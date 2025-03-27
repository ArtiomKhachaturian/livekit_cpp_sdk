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
#ifdef WEBRTC_AVAILABLE
#include "Loggable.h"
#include "Listener.h"
#include "Participant.h"
#include "CameraTrackImpl.h"
#include "LocalAudioTrackImpl.h"
#include "ParticipantListener.h"
#include "rtc/ParticipantInfo.h"
#include "AesCgmCryptorObserver.h"
#include "SafeObj.h"
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

class CameraVideoTrack;
class TrackManager;
class ParticipantListener;
class PeerConnectionFactory;
struct TrackPublishedResponse;
struct TrackUnpublishedResponse;
struct MicrophoneOptions;

class LocalParticipant : public Bricks::LoggableS<Participant, AesCgmCryptorObserver>
{
    using Base = Bricks::LoggableS<Participant, AesCgmCryptorObserver>;
    template<class T> using Tracks = Bricks::SafeObj<std::vector<std::shared_ptr<T>>>;
public:
    LocalParticipant(TrackManager* manager, PeerConnectionFactory* pcf,
                     const Participant* session,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipant() final = default;
    void reset() { _session(nullptr); }
    std::optional<bool> stereoRecording() const;
    size_t audioTracksCount() const;
    size_t videoTracksCount() const;
    std::shared_ptr<LocalAudioTrackImpl> addMicrophoneTrack(const MicrophoneOptions& options);
    std::shared_ptr<CameraTrackImpl> addCameraTrack();
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeAudioTrack(const std::shared_ptr<AudioTrack>& track);
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeVideoTrack(const std::shared_ptr<VideoTrack>& track);
    std::shared_ptr<AudioTrack> audioTrack(size_t index) const;
    std::shared_ptr<VideoTrack> videoTrack(size_t index) const;
    std::vector<std::shared_ptr<LocalTrack>> tracks() const;
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> media() const;
    std::shared_ptr<LocalTrack> track(const std::string& id, bool cid,
                                      const std::optional<cricket::MediaType>& hint = {}) const;
    std::shared_ptr<LocalTrack> track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const;
    void setListener(ParticipantListener* listener) { _listener = listener; }
    void setInfo(const ParticipantInfo& info);
    void enableAesCgmForLocalMedia(bool enable) { _aesCgmEnabledForLocalMedia = enable; }
    bool aesCgmEnabledForLocalMedia() const { return _aesCgmEnabledForLocalMedia; }
    // impl. of Participant
    std::string sid() const final { return _sid(); }
    std::string identity() const final { return _identity(); }
    std::string name() const final { return _name(); }
    std::string metadata() const final { return _metadata(); }
    ParticipantKind kind() const final { return _kind; }
private:
    void addTrack(const std::shared_ptr<LocalAudioTrackImpl>& track);
    void addTrack(const std::shared_ptr<VideoTrack>& track);
    std::shared_ptr<LocalTrack> lookupAudio(const std::string& id, bool cid) const;
    std::shared_ptr<LocalTrack> lookupVideo(const std::string& id, bool cid) const;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createMic(const MicrophoneOptions& options) const;
    webrtc::scoped_refptr<CameraVideoTrack> createCamera() const;
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const;
    // impl. of AesCgmCryptorObserver
    void onEncryptionStateChanged(cricket::MediaType mediaType, const std::string&,
                                  const std::string& trackId, AesCgmCryptorState state) final;
private:
    TrackManager* const _manager;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    Bricks::SafeObj<const Participant*> _session;
    Bricks::Listener<ParticipantListener*> _listener;
    Tracks<LocalAudioTrackImpl> _audioTracks;
    Tracks<VideoTrack> _videoTracks;
    Bricks::SafeObj<std::string> _sid;
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _name;
    Bricks::SafeObj<std::string> _metadata;
    std::atomic<ParticipantKind> _kind = ParticipantKind::Standard;
    std::atomic_bool _aesCgmEnabledForLocalMedia = true;
};

} // namespace LiveKitCpp
#endif
