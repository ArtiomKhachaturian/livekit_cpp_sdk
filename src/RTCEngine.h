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
#pragma once // RTCEngine.h
#ifdef WEBRTC_AVAILABLE
#include "RTCMediaEngine.h"
#include "DataChannelsStorage.h"
#include "DataExchangeListener.h"
#include "Options.h"
#include "SafeObj.h"
#include "SignalClientWs.h"
#include "SignalTransportListener.h"
#include "SessionState.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/DisconnectReason.h"
#include "rtc/JoinResponse.h"
#include "rtc/LeaveRequestAction.h"
#include <atomic>
#include <string>
#include <vector>

namespace Websocket {
class EndPoint;
}

namespace webrtc {
class DataChannelInterface;
class RtpSenderInterface;
}

namespace LiveKitCpp
{

class TransportManager;

// https://github.com/livekit/client-sdk-js/blob/main/src/room/RTCEngine.ts
class RTCEngine : public RTCMediaEngine,
                  private SignalTransportListener,
                  private DataExchangeListener
{
public:
    RTCEngine(Options options,
              PeerConnectionFactory* pcf,
              const Participant* session,
              std::unique_ptr<Websocket::EndPoint> socket,
              const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCEngine() final;
    SessionState state() const noexcept { return _state; }
    bool connect(std::string url, std::string authToken);
    void disconnect();
    bool sendUserPacket(std::string payload, bool reliable,
                        const std::vector<std::string>& destinationIdentities = {},
                        const std::string& topic = {}) const;
    bool sendChatMessage(std::string message, bool deleted) const;
    // override of RTCMediaEngine
    std::shared_ptr<LocalAudioTrackImpl> addLocalMicrophoneTrack(const MicrophoneOptions& options) final;
    std::shared_ptr<CameraTrackImpl> addLocalCameraTrack() final;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeLocalAudioTrack(const std::shared_ptr<AudioTrack>& track) final;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        removeLocalVideoTrack(const std::shared_ptr<VideoTrack>& track) final;
protected:
    // impl. or overrides of RTCMediaEngine
    SendResult sendAddTrack(const AddTrackRequest& request) const final;
    SendResult sendMuteTrack(const MuteTrackRequest& request) const final;
    SendResult sendUpdateLocalAudioTrack(const UpdateLocalAudioTrack& request) const final;
    bool closed() const final;
    void cleanup(const std::optional<LiveKitError>& error = {},
                 const std::string& errorDetails = {}) final;
private:
    void sendLeave(DisconnectReason reason = DisconnectReason::ClientInitiated,
                   LeaveRequestAction action = LeaveRequestAction::Disconnect) const;
    template <class ReqMethod, class TReq>
    SendResult sendRequestToServer(const ReqMethod& method, const TReq& req) const;
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
    // impl. of TransportManagerListener
    void onSdpOperationFailed(SignalTarget, webrtc::RTCError error) final;
    void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState state,
                       webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                       webrtc::PeerConnectionInterface::PeerConnectionState subscriberState) final;
    void onNegotiationNeeded() final;
    void onPublisherOffer(const webrtc::SessionDescriptionInterface* desc) final;
    void onSubscriberAnswer(const webrtc::SessionDescriptionInterface* desc) final;
    void onIceCandidateGathered(SignalTarget target,
                                const webrtc::IceCandidateInterface* candidate) final;
    void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel) final;
    void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel) final;
    // impl. of SignalServerListener
    void onJoin(const JoinResponse& response) final;
    void onReconnect(const ReconnectResponse& response) final;
    void onOffer(const SessionDescription& sdp) final;
    void onAnswer(const SessionDescription& sdp) final;
    void onPong(const Pong& pong) final;
    void onTrickle(const TrickleRequest& request) final;
    void onLeave(const LeaveRequest& leave) final;
    void onTrackUnpublished(const TrackUnpublishedResponse& unpublished) final;
    // impl. of SignalTransportListener
    void onTransportStateChanged(TransportState state) final;
    void onTransportError(std::string error) final;
    // impl. of PingPongKitListener
    bool onPingRequested() final;
    void onPongTimeout() final;
    // impl. of DataExchangeListener
    void onUserPacket(const std::string& participantSid,
                      const std::string& participantIdentity,
                      const std::string& payload,
                      const std::vector<std::string>& /*destinationIdentities*/,
                      const std::string& topic) final;
    void onChatMessage(const std::string& remoteParticipantIdentity,
                       const std::string& message, const std::string& id,
                       int64_t timestamp, bool deleted, bool generated) final;
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    const Options _options;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    DataChannelsStorage _localDcs;
    DataChannelsStorage _remoteDcs;
    SignalClientWs _client;
    std::shared_ptr<TransportManager> _pcManager;
    /** keeps track of how often an initial join connection has been tried */
    std::atomic_uint _reconnectAttempts = 0U;
    std::atomic<SessionState> _state = SessionState::TransportDisconnected;
    Bricks::SafeObj<JoinResponse> _lastJoinResponse;
};

} // namespace LiveKitCpp
#endif
