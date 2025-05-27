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
#pragma once // RTCEngineImpl.h
#include "Listener.h"
#include "Loggable.h"
#include "TrackManager.h"
#include "TransportManagerListener.h"
#include "RemoteParticipants.h"
#include "RemoteParticipantsListener.h"
#include "DataChannelsStorage.h"
#include "DataExchangeListener.h"
#include "SafeObj.h"
#include "livekit/rtc/LiveKitError.h"
#include "livekit/signaling/ResponsesListener.h"
#include "livekit/rtc/SessionState.h"
#include "livekit/rtc/Options.h"
#include "livekit/signaling/SignalClientWs.h"
#include "livekit/signaling/SignalTransportListener.h"
#include "livekit/signaling/sfu/ClientConfiguration.h"
#include "livekit/signaling/sfu/DisconnectReason.h"
#include "livekit/signaling/sfu/JoinResponse.h"
#include "livekit/signaling/sfu/LeaveRequestAction.h"
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace webrtc {
class Thread;
class DataChannelInterface;
class RtpSenderInterface;
}

namespace Websocket {
class EndPoint;
}

namespace LiveKitCpp
{

class AudioTrack;
class AudioDevice;
class AesCgmCryptor;
class LocalVideoDevice;
class CameraTrackImpl;
class KeyProvider;
class LocalParticipant;
class LocalTrackAccessor;
class LocalAudioTrack;
class LocalVideoTrack;
class LocalAudioTrackImpl;
class LocalVideoTrackImpl;
class Participant;
class PeerConnectionFactory;
class ParticipantAccessor;
class SessionListener;
class SignalTransportListenerAsync;
class TransportManager;
class VideoDevice;
class VideoTrack;
struct AddTrackRequest;
struct MuteTrackRequest;
struct UpdateLocalAudioTrack;
enum class DisconnectReason;
enum class EncryptionType;

class RTCEngineImpl : public std::enable_shared_from_this<RTCEngineImpl>,
                      public TrackManager,
                      private Bricks::LoggableS<ResponsesListener>,
                      private TransportManagerListener,
                      private RemoteParticipantsListener,
                      private SignalTransportListener,
                      private DataExchangeListener
{
    enum class SendResult;
public:
    RTCEngineImpl(Options options, bool disableAudioRed,
                  PeerConnectionFactory* pcf,
                  const Participant* session,
                  std::unique_ptr<Websocket::EndPoint> socket,
                  const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCEngineImpl() final;
    void setListener(SessionListener* listener);
    const auto& localParticipant() const noexcept { return _localParticipant; }
    const auto& remoteParticipants() const noexcept { return _remoteParicipants; }
    std::string addTrackDevice(std::unique_ptr<AudioDevice> device, EncryptionType encryption);
    std::string addTrackDevice(std::unique_ptr<LocalVideoDevice> device, EncryptionType encryption);
    void removeTrackDevice(const std::string& deviceId);
    void setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider = {});
    void setAudioPlayout(bool playout);
    bool audioPlayoutEnabled() const { return _playout; }
    void setAudioRecording(bool recording);
    bool audioRecordingEnabled() const { return _recording; }
    SessionState state() const noexcept { return _state; }
    bool connect(std::string url, std::string authToken);
    void disconnect();
    bool sendUserPacket(std::string payload, bool reliable,
                        const std::string& topic = {},
                        const std::vector<std::string>& destinationSids = {},
                        const std::vector<std::string>& destinationIdentities = {}) const;
    bool sendChatMessage(std::string message,
                         bool deleted,
                         bool generated,
                         const std::vector<std::string>& destinationIdentities = {}) const;
    void queryStats(const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void cleanup(const std::optional<LiveKitError>& error = {}, const std::string& errorDetails = {});
private:
   // webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> localTrack(const std::string& id, bool cid) const;
    SendResult sendAddTrack(AddTrackRequest request) const;
    SendResult sendMuteTrack(MuteTrackRequest request) const;
    SendResult sendUpdateLocalAudioTrack(UpdateLocalAudioTrack request) const;
    bool closed() const;
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    std::shared_ptr<ParticipantAccessor> participant(const std::string& sid) const;
    void handleLocalParticipantDisconnection(DisconnectReason reason);
    void notifyAboutLocalParticipantJoinLeave(bool join);
    // search by cid or sid
    template <class TTrack>
    bool sendAddTrack(const std::shared_ptr<TTrack>& track);
    void sendLeave(DisconnectReason reason = DisconnectReason::ClientInitiated,
                   LeaveRequestAction action = LeaveRequestAction::Disconnect) const;
    template <class ReqMethod, class TReq>
    SendResult sendRequestToServer(const ReqMethod& method, TReq req) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const std::vector<ICEServer>& iceServers = {},
                          const std::optional<ClientConfiguration>& cc = {}) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const JoinResponse& response) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const ReconnectResponse& response) const;
    void changeState(SessionState state);
    void changeState(webrtc::PeerConnectionInterface::PeerConnectionState state);
    void changeState(TransportState state);
    void createTransportManager(const JoinResponse& response,
                                const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
    // impl. of TrackManager
    void queryStats(const webrtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                    const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const final;
    void queryStats(const webrtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                    const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const final;
    webrtc::scoped_refptr<webrtc::FrameTransformerInterface> createCryptor(EncryptionType encryption,
                                                                           webrtc::MediaType mediaType,
                                                                           std::string identity, std::string trackId,
                                                                           const std::weak_ptr<AesCgmCryptorObserver>& observer) const final;
    // overrides of RemoteParticipantsListener
    void onUpdateSubscription(UpdateSubscription subscription) final;
    // impl. TrackManager
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
    void notifyAboutSetRtpParametersFailure(const std::string& trackSid, std::string_view details) final;
    std::optional<bool> stereoRecording() const final;
    // impl. of RemoteParticipantsListener
    void onParticipantAdded(const std::string& sid) final;
    void onParticipantRemoved(const std::string& sid) final;
    // impl. of SignalServerListener
    void onJoin(JoinResponse response) final;
    void onUpdate(ParticipantUpdate update) final;
    void onTrackPublished(TrackPublishedResponse published) final;
    void onReconnect(ReconnectResponse response) final;
    void onMute(MuteTrackRequest mute) final;
    // always received StreamState::Active from the SFU, maybe this is a bug
    void onStreamStateUpdate(StreamStateUpdate /*update*/) final {}
    void onSpeakersChanged(SpeakersChanged changed) final;
    void onConnectionQuality(ConnectionQualityUpdate update) final;
    void onOffer(SessionDescription sdp) final;
    void onAnswer(SessionDescription sdp) final;
    void onPong(Pong pong) final;
    void onTrickle(TrickleRequest request) final;
    void onLeave(LeaveRequest leave) final;
    void onTrackUnpublished(TrackUnpublishedResponse unpublished) final;
    void onRefreshToken(std::string authToken) final;
    // impl. of TransportManagerListener
    void onLocalAudioTrackAdded(const std::shared_ptr<LocalAudioTrackImpl>& track) final;
    void onLocalVideoTrackAdded(const std::shared_ptr<LocalVideoTrackImpl>& track) final;
    void onLocalTrackAddFailure(std::string id, webrtc::MediaType type, webrtc::RTCError error) final;
    void onLocalTrackRemoved(std::string id, webrtc::MediaType) final;
    void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                       webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                       webrtc::PeerConnectionInterface::PeerConnectionState subscriberState) final;
    void onRemoteTrackAdded(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            std::string trackId, std::string participantSid) final;
    void onRemotedTrackRemoved(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    void onSdpOperationFailed(SignalTarget, webrtc::RTCError error) final;
    void onNegotiationNeeded() final;
    void onPublisherOffer(std::string type, std::string sdp) final;
    void onSubscriberAnswer(std::string type, std::string sdp) final;
    void onIceCandidateGathered(SignalTarget target, std::string sdpMid,
                                int sdpMlineIndex, webrtc::Candidate candidate) final;
    void onLocalDataChannelCreated(webrtc::scoped_refptr<DataChannel> channel) final;
    void onRemoteDataChannelOpened(webrtc::scoped_refptr<DataChannel> channel) final;
    // impl. of SignalTransportListener
    void onTransportStateChanged(TransportState state) final;
    void onTransportError(std::string error) final;
    // impl. of PingPongKitListener
    bool onPingRequested() final;
    void onPongTimeout() final;
    // impl. of DataExchangeListener
    void onUserPacket(UserPacket packet, std::string participantIdentity,
                      std::vector<std::string> destinationIdentities) final;
    void onChatMessage(ChatMessage message, std::string participantIdentity,
                       std::vector<std::string> destinationIdentities) final;
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    const Options _options;
    const bool _disableAudioRed;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    const std::shared_ptr<LocalParticipant> _localParticipant;
    const std::shared_ptr<RemoteParticipants> _remoteParicipants;
    const std::unique_ptr<SignalTransportListenerAsync> _transportListener;
    Bricks::Listener<SessionListener*> _listener;
    Bricks::SafeObj<std::string> _sifTrailer;
    std::shared_ptr<KeyProvider> _aesCgmKeyProvider;
    DataChannelsStorage _localDcs;
    DataChannelsStorage _remoteDcs;
    SignalClientWs _client;
    std::atomic_bool _playout = true;
    std::atomic_bool _recording = true;
    std::shared_ptr<TransportManager> _pcManager;
    /** keeps track of how often an initial join connection has been tried */
    std::atomic_uint _reconnectAttempts = 0U;
    std::atomic<SessionState> _state = SessionState::TransportDisconnected;
    Bricks::SafeObj<JoinResponse> _lastJoinResponse;
    std::atomic_bool _joined = false;
};
	
} // namespace LiveKitCpp
