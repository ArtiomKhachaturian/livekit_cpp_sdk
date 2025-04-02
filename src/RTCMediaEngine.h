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
#pragma once // RTCMediaEngine.h
#ifdef WEBRTC_AVAILABLE
#include "Listener.h"
#include "Loggable.h"
#include "E2ESecurityFactory.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "TransportManagerListener.h"
#include "RemoteParticipants.h"
#include "RemoteParticipantsListener.h"
#include "LiveKitError.h"
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace rtc {
class Thread;
}

namespace LiveKitCpp
{

class AudioTrack;
class AesCgmCryptor;
class CameraTrackImpl;
class KeyProvider;
class LocalParticipant;
class LocalTrack;
class LocalAudioTrackImpl;
class Participant;
class PeerConnectionFactory;
class SessionListener;
class VideoTrack;
struct AddTrackRequest;
struct CameraOptions;
struct MuteTrackRequest;
struct UpdateLocalAudioTrack;
enum class DisconnectReason;

class RTCMediaEngine : public Bricks::LoggableS<SignalServerListener>,
                       protected TransportManagerListener,
                       protected E2ESecurityFactory,
                       private RemoteParticipantsListener
{
public:
    void setListener(SessionListener* listener);
    const Participant& localParticipant() const noexcept;
    const auto& remoteParticipants() const noexcept { return _remoteParicipants; }
    size_t localAudioTracksCount() const;
    size_t localVideoTracksCount() const;
    virtual std::shared_ptr<LocalAudioTrackImpl> addLocalMicrophoneTrack();
    virtual std::shared_ptr<CameraTrackImpl> addLocalCameraTrack(const CameraOptions& options);
    virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeLocalAudioTrack(const std::shared_ptr<AudioTrack>& track);
    virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeLocalVideoTrack(const std::shared_ptr<VideoTrack>& track);
    std::shared_ptr<AudioTrack> localAudioTrack(size_t index) const;
    std::shared_ptr<VideoTrack> localVideoTrack(size_t index) const;
    void enableAesCgmForLocalMedia(bool enable);
    bool aesCgmEnabledForLocalMedia() const noexcept;
    void setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider = {});
protected:
    enum class SendResult
    {
        Ok,
        TransportError,
        TransportClosed
    };
protected:
    RTCMediaEngine(PeerConnectionFactory* pcf, const Participant* session,
                   const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCMediaEngine() override;
    void resetLocalParticipant();
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> localTracks() const;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> localTrack(const std::string& id, bool cid) const;
    virtual SendResult sendAddTrack(const AddTrackRequest& request) const = 0;
    virtual SendResult sendMuteTrack(const MuteTrackRequest& request) const = 0;
    virtual SendResult sendUpdateLocalAudioTrack(const UpdateLocalAudioTrack& request) const = 0;
    virtual bool closed() const = 0;
    virtual void cleanup(const std::optional<LiveKitError>& error = {},
                         const std::string& errorDetails = {});
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const {
        _listener.invoke(method, std::forward<Args>(args)...);
    }
    // impl. of SignalServerListener
    void onJoin(const JoinResponse& response) override;
    void onUpdate(const ParticipantUpdate& update) override;
    void onTrackPublished(const TrackPublishedResponse& published) override;
    void onReconnect(const ReconnectResponse& response) override;
    // impl. of TransportManagerListener
    void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) override;
    void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                       webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                       webrtc::PeerConnectionInterface::PeerConnectionState subscriberState) override;
    void onLocalTrackRemoved(const std::string& id, cricket::MediaType) override;
    void onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
    void onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
private:
    void handleLocalParticipantDisconnection(DisconnectReason reason);
    void notifyAboutLocalParticipantJoinLeave(bool join) const;
    // search by cid or sid
    void sendAddTrack(const std::shared_ptr<LocalTrack>& track);
    // impl. of FrameCodecFactory
    webrtc::scoped_refptr<AesCgmCryptor> createCryptor(bool local,
                                                       cricket::MediaType mediaType,
                                                       std::string identity,
                                                       std::string trackId) const final;
    // impl. TrackManager
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
    std::optional<bool> stereoRecording() const final;
    EncryptionType localEncryptionType() const final;
    // impl. of RemoteParticipantsListener
    void onParticipantAdded(const std::string& sid) final;
    void onParticipantRemoved(const std::string& sid) final;
private:
    const std::weak_ptr<rtc::Thread> _signalingThread;
    const std::shared_ptr<LocalParticipant> _localParticipant;
    RemoteParticipants _remoteParicipants;
    Bricks::Listener<SessionListener*> _listener;
    Bricks::SafeObj<std::string> _sifTrailer;
    std::shared_ptr<KeyProvider> _aesCgmKeyProvider;
};

} // namespace LiveKitCpp
#endif
