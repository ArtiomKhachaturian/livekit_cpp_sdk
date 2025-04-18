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
#include "SafeScopedRefPtr.h"
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
class LocalVideoTrack;
class LocalAudioTrackImpl;
class LocalVideoTrackImpl;
class Participant;
class PeerConnectionFactory;
class ParticipantAccessor;
class SessionListener;
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
    enum class SendResult
    {
        Ok,
        TransportError,
        TransportClosed
    };
public:
    RTCEngineImpl(Options options,
                  PeerConnectionFactory* pcf,
                  const Participant* session,
                  std::unique_ptr<Websocket::EndPoint> socket,
                  const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCEngineImpl() final;
    void setListener(SessionListener* listener);
    const auto& localParticipant() const noexcept { return _localParticipant; }
    const auto& remoteParticipants() const noexcept { return _remoteParicipants; }
    std::shared_ptr<LocalAudioTrackImpl> addLocalAudioTrack(std::shared_ptr<AudioDevice> device,
                                                            EncryptionType encryption);
    std::shared_ptr<LocalVideoTrackImpl> addLocalVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                            EncryptionType encryption);
    bool removeLocalAudioTrack(std::shared_ptr<AudioTrack> track);
    bool removeLocalVideoTrack(std::shared_ptr<LocalVideoTrack> track);
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
    void queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void cleanup(const std::optional<LiveKitError>& error = {}, const std::string& errorDetails = {});
private:
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> localTrack(const std::string& id, bool cid) const;
    SendResult sendAddTrack(AddTrackRequest request) const;
    SendResult sendMuteTrack(MuteTrackRequest request) const;
    SendResult sendUpdateLocalAudioTrack(UpdateLocalAudioTrack request) const;
    bool closed() const;
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    std::shared_ptr<ParticipantAccessor> participant(const std::string& sid) const;
    void handleLocalParticipantDisconnection(DisconnectReason reason);
    void notifyAboutLocalParticipantJoinLeave(bool join) const;
    // search by cid or sid
    void sendAddTrack(const std::shared_ptr<LocalTrackAccessor>& track);
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
    void queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                    const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const final;
    void queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                    const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const final;
    webrtc::scoped_refptr<webrtc::FrameTransformerInterface> createCryptor(EncryptionType encryption,
                                                                           cricket::MediaType mediaType,
                                                                           std::string identity, std::string trackId,
                                                                           const std::weak_ptr<AesCgmCryptorObserver>& observer) const final;
    // overrides of RemoteParticipantsListener
    void onUpdateSubscription(UpdateSubscription subscription) final;
    // impl. TrackManager
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
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
    void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) final;
    void onLocalTrackRemoved(std::string id, cricket::MediaType) final;
    void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                       webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                       webrtc::PeerConnectionInterface::PeerConnectionState subscriberState) final;
    void onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            std::string trackId, std::string participantSid) final;
    void onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    void onSdpOperationFailed(SignalTarget, webrtc::RTCError error) final;
    void onNegotiationNeeded() final;
    void onPublisherOffer(std::string type, std::string sdp) final;
    void onSubscriberAnswer(std::string type, std::string sdp) final;
    void onIceCandidateGathered(SignalTarget target, std::string sdpMid,
                                int sdpMlineIndex, cricket::Candidate candidate) final;
    void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel) final;
    void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel) final;
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
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    const std::shared_ptr<LocalParticipant> _localParticipant;
    const std::shared_ptr<RemoteParticipants> _remoteParicipants;
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
};
	
} // namespace LiveKitCpp
