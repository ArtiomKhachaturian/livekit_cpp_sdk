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
#ifdef WEBRTC_AVAILABLE
#include "RTCEngine.h"
#include "LocalParticipant.h"
#include "TransportManager.h"
#include "PeerConnectionFactory.h"
#include "WebsocketEndPoint.h"
#include "RoomUtils.h"
#include "SessionListener.h"
#include "Utils.h"
#include "rtc/SignalTarget.h"
#include "rtc/ReconnectResponse.h"
#include "rtc/Ping.h"
#include "rtc/TrickleRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/LeaveRequest.h"
#include "rtc/TrackUnpublishedResponse.h"
#include <thread>

namespace {

inline constexpr bool failed(webrtc::PeerConnectionInterface::PeerConnectionState state) {
    return webrtc::PeerConnectionInterface::PeerConnectionState::kFailed == state;
}

}

namespace LiveKitCpp
{

RTCEngine::RTCEngine(Options options,
                     PeerConnectionFactory* pcf,
                     const Participant* session,
                     std::unique_ptr<Websocket::EndPoint> socket,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : RTCMediaEngine(pcf, session, logger)
    , _options(std::move(options))
    , _pcf(pcf)
    , _localDcs(logger)
    , _remoteDcs(logger)
    , _client(std::move(socket), logger.get())
{
    _client.setAdaptiveStream(_options._adaptiveStream);
    _client.setAutoSubscribe(_options._autoSubscribe);
    _client.setPublish(_options._publish);
    _client.setClientInfo(_options._clientsInfo);
    _client.setServerListener(this);
    _client.setTransportListener(this);
}

RTCEngine::~RTCEngine()
{
    _client.setServerListener(nullptr);
    _client.setTransportListener(nullptr);
    cleanup();
}

bool RTCEngine::connect(std::string url, std::string authToken)
{
    if (url.empty()) {
        logError("server URL is empty");
        return false;
    }
    if (authToken.empty()) {
        logError("authentification token is empty");
        return false;
    }
    _client.setHost(std::move(url));
    _client.setAuthToken(std::move(authToken));
    return _client.connect();
}

void RTCEngine::disconnect()
{
    if (auto pcManager = std::atomic_exchange(&_pcManager, std::shared_ptr<TransportManager>())) {
        pcManager->stopPing();
        // send leave event
        sendLeave();
        _client.disconnect();
        pcManager->close();
    }
}

bool RTCEngine::sendUserPacket(std::string payload,
                               bool reliable,
                               const std::vector<std::string>& destinationIdentities,
                               const std::string& topic) const
{
    return _localDcs.sendUserPacket(std::move(payload), reliable, destinationIdentities, topic);
}

bool RTCEngine::sendChatMessage(std::string message, bool deleted) const
{
    return _localDcs.sendChatMessage(std::move(message), deleted);
}

std::shared_ptr<LocalAudioTrackImpl> RTCEngine::
    addLocalMicrophoneTrack(const MicrophoneOptions& options)
{
    const auto track = RTCMediaEngine::addLocalMicrophoneTrack(options);
    if (track) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->addTrack(track->media());
        }
    }
    return track;
}

std::shared_ptr<CameraTrackImpl> RTCEngine::addLocalCameraTrack()
{
    const auto track = RTCMediaEngine::addLocalCameraTrack();
    if (track) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->addTrack(track->media());
        }
    }
    return track;
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngine::
    removeLocalAudioTrack(const std::shared_ptr<AudioTrack>& track)
{
    const auto media = RTCMediaEngine::removeLocalAudioTrack(track);
    if (media) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->removeTrack(media);
        }
    }
    return media;
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngine::
    removeLocalVideoTrack(const std::shared_ptr<VideoTrack>& track)
{
    const auto media = RTCMediaEngine::removeLocalVideoTrack(track);
    if (media) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->removeTrack(media);
        }
    }
    return media;
}

RTCEngine::SendResult RTCEngine::sendAddTrack(const AddTrackRequest& request) const
{
    return sendRequestToServer(&SignalClient::sendAddTrack, request);
}

RTCEngine::SendResult RTCEngine::sendMuteTrack(const MuteTrackRequest& request) const
{
    return sendRequestToServer(&SignalClient::sendMuteTrack, request);
}

RTCEngine::SendResult RTCEngine::sendUpdateLocalAudioTrack(const UpdateLocalAudioTrack& request) const
{
    return sendRequestToServer(&SignalClient::sendUpdateAudioTrack, request);
}

void RTCEngine::cleanup(const std::optional<LiveKitError>& error,
                        const std::string& errorDetails)
{
    RTCMediaEngine::cleanup(error, errorDetails);
    if (auto pcManager = std::atomic_exchange(&_pcManager, std::shared_ptr<TransportManager>())) {
        pcManager->setListener(nullptr);
    }
    _client.disconnect();
    _localDcs.setListener(nullptr);
    _remoteDcs.setListener(nullptr);
    if (error) {
        callback(&SessionListener::onError, error.value(), errorDetails);
    }
}

void RTCEngine::sendLeave(DisconnectReason reason, LeaveRequestAction action) const
{
    LeaveRequest request;
    request._reason = reason;
    request._action = action;
    sendRequestToServer(&SignalClient::sendLeave, request);
}

bool RTCEngine::closed() const
{
    if (TransportState::Connected != _client.transportState()) {
        return true;
    }
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        return pcManager->closed();
    }
    return true;
}

template <class ReqMethod, class TReq>
RTCEngine::SendResult RTCEngine::sendRequestToServer(const ReqMethod& method, const TReq& req) const
{
    if (!closed()) {
        if ((_client.*method)(req)) {
            return SendResult::Ok;
        }
        return SendResult::TransportError;
    }
    return SendResult::TransportClosed;
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngine::
    makeConfiguration(const std::vector<ICEServer>& iceServers,
                      const std::optional<ClientConfiguration>& cc) const
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    //config.network_preference.emplace(rtc::AdapterType::ADAPTER_TYPE_ETHERNET); // ethernet is preferred
    // set some defaults
    //config.set_cpu_adaptation(true);
    //config.enable_dtls_srtp.emplace(true);
    // enable ICE renomination, like on Android (   "a=ice-options:trickle renomination")
    //config.enable_ice_renomination = true;
    // required for M86:
    // the issue may because 1 byte rtp header id exahustion, check if you have enabled this option:
    // https://source.chromium.org/chromium/chromium/src/+/master:third_party/webrtc/api/peer_connection_interface.h;l=625?q=RTCConfiguration%20webrtc&ss=chromium%2Fchromium%2Fsrc
    config.offer_extmap_allow_mixed = true;
    if (cc.has_value() && ClientConfigSetting::Enabled  == cc->_forceRelay) {
        config.type = webrtc::PeerConnectionInterface::kRelay;
    }
    else {
        config.type = RoomUtils::map(_options._iceTransportPolicy);
    }
    if (!_options._iceServers.empty()) {
        // Override with user provided iceServers
        config.servers = RoomUtils::map(_options._iceServers);
    }
    else {
        //Set iceServers provided by the server
        config.servers = RoomUtils::map(iceServers);
    }
    config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
    // crypto options
    /*{
        webrtc::CryptoOptions crypto_options;
        // enables GCM suites for DTLS-SRTP
        crypto_options.srtp.enable_gcm_crypto_suites = true;
        crypto_options.srtp.enable_aes128_sha1_32_crypto_cipher = false;
        crypto_options.srtp.enable_aes128_sha1_80_crypto_cipher = false;
        crypto_options.srtp.enable_encrypted_rtp_header_extensions = false;
        // enforces frame encryption on RTP senders and receivers
        //crypto_options.sframe.require_frame_encryption = requireFrameEncryption;
        config.crypto_options.emplace(std::move(crypto_options));
    }*/
    return config;
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngine::
    makeConfiguration(const JoinResponse& response) const
{
    return makeConfiguration(response._iceServers, response._clientConfiguration);
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngine::
    makeConfiguration(const ReconnectResponse& response) const
{
    return makeConfiguration(response._iceServers, response._clientConfiguration);
}

void RTCEngine::changeState(SessionState state)
{
    if (state != _state.exchange(state)) {
        callback(&SessionListener::onStateChanged, state);
    }
}

void RTCEngine::changeState(webrtc::PeerConnectionInterface::PeerConnectionState state)
{
    switch (state) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
            changeState(SessionState::RtcConnecting);
            break;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
            changeState(SessionState::RtcConnected);
            break;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
            changeState(SessionState::RtcDisconnected);
            break;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            changeState(SessionState::RtcClosed);
            break;
        default:
            break;
    }
}

void RTCEngine::changeState(TransportState state)
{
    switch (state) {
        case TransportState::Connecting:
            changeState(SessionState::TransportConnecting);
            break;
        case TransportState::Connected:
            changeState(SessionState::TransportConnected);
            break;
        case TransportState::Disconnecting:
            changeState(SessionState::TransportDisconnecting);
            break;
        case TransportState::Disconnected:
            changeState(SessionState::TransportDisconnected);
            break;
        default:
            break;
    }
}

void RTCEngine::createTransportManager(const JoinResponse& response,
                                       const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
{
    auto pcManager = std::make_shared<TransportManager>(response._subscriberPrimary,
                                                        response._fastPublish,
                                                        response._pingTimeout,
                                                        response._pingInterval,
                                                        _pcf, conf,
                                                        response._participant._identity,
                                                        logger());
    pcManager->setListener(this);
    _reconnectAttempts = 0U;
    std::atomic_store(&_pcManager, pcManager);
    for (auto track : localTracks()) {
        pcManager->addTrack(std::move(track));
    }
    pcManager->negotiate(false);
    pcManager->startPing();
}

void RTCEngine::onSdpOperationFailed(SignalTarget, webrtc::RTCError error)
{
    sendLeave();
    cleanup(LiveKitError::RTC, error.message());
}

void RTCEngine::onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState state,
                              webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                              webrtc::PeerConnectionInterface::PeerConnectionState subscriberState)
{
    RTCMediaEngine::onStateChange(state, publisherState, subscriberState);
    if (failed(state) || failed(publisherState) || failed(subscriberState)) {
        cleanup(LiveKitError::RTC);
    }
    else {
        changeState(state);
    }
}

void RTCEngine::onNegotiationNeeded()
{
    RTCMediaEngine::onNegotiationNeeded();
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        pcManager->negotiate(true);
    }
}

void RTCEngine::onPublisherOffer(const webrtc::SessionDescriptionInterface* desc)
{
    RTCMediaEngine::onPublisherOffer(desc);
    if (const auto sdp = RoomUtils::map(desc)) {
        if (!_client.sendOffer(sdp.value())) {
            logError("failed to send publisher offer to the server");
        }
        else {
            logInfo("publisher offer has been sent to server");
        }
    }
    else {
        logError("failed to serialize publisher offer into a string");
    }
}

void RTCEngine::onSubscriberAnswer(const webrtc::SessionDescriptionInterface* desc)
{
    RTCMediaEngine::onSubscriberAnswer(desc);
    if (const auto sdp = RoomUtils::map(desc)) {
        if (!_client.sendAnswer(sdp.value())) {
            logError("failed to send subscriber answer to the server");
        }
        else {
            logInfo("subscriber answer has been sent to server");
        }
    }
    else {
        logError("failed to serialize subscriber answer into a string");
    }
}

void RTCEngine::onIceCandidateGathered(SignalTarget target,
                                       const webrtc::IceCandidateInterface* candidate)
{
    RTCMediaEngine::onIceCandidateGathered(target, candidate);
    if (candidate) {
        auto candidateInit = RoomUtils::map(candidate);
        if (candidateInit.empty()) {
            logError("failed to serialize " + candidate->server_url() +
                     " " + toString(target) + " local ICE candidate");
            return;
        }
        TrickleRequest request;
        request._candidateInit = std::move(candidateInit);
        request._target = target;
        request._final = false;
        if (!_client.sendTrickle(request)) {
            logError("failed to send " + candidate->server_url() +
                     " " + toString(target) + " local ICE candidate");
        }
    }
}

void RTCEngine::onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel)
{
    _localDcs.add(std::move(channel));
}

void RTCEngine::onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel)
{
    _remoteDcs.add(std::move(channel));
}

void RTCEngine::onJoin(const JoinResponse& response)
{
    RTCMediaEngine::onJoin(response);
    if (DisconnectReason::UnknownReason == response._participant._disconnectReason) {
        _localDcs.setSid(response._participant._sid);
        _localDcs.setIdentity(response._participant._identity);
        _lastJoinResponse(response);
        createTransportManager(response, makeConfiguration(response));
    }
}

void RTCEngine::onReconnect(const ReconnectResponse& response)
{
    RTCMediaEngine::onReconnect(response);
    createTransportManager(_lastJoinResponse(), makeConfiguration(response));
}

void RTCEngine::onOffer(const SessionDescription& sdp)
{
    RTCMediaEngine::onOffer(sdp);
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setRemoteOffer(std::move(desc));
        }
    }
    else  {
        logError("failed to parse remote offer SDP: " + error.description);
    }
}

void RTCEngine::onAnswer(const SessionDescription& sdp)
{
    RTCMediaEngine::onAnswer(sdp);
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setRemoteAnswer(std::move(desc));
        }
    }
    else {
        logError("failed to parse remote answer SDP: " + error.description);
    }
}

void RTCEngine::onPong(const Pong& pong)
{
    RTCMediaEngine::onPong(pong);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        // Clear timeout timer
        pcManager->notifyThatPongReceived();
    }
}

void RTCEngine::onTrickle(const TrickleRequest& request)
{
    RTCMediaEngine::onTrickle(request);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        webrtc::SdpParseError error;
        if (auto candidate = RoomUtils::map(request, &error)) {
            pcManager->addIceCandidate(request._target, std::move(candidate));
        }
        else {
            logError("failed to parse ICE candidate SDP for " +
                     toString(request._target) + ": " + error.description);
        }
    }
}

void RTCEngine::onLeave(const LeaveRequest& leave)
{
    RTCMediaEngine::onLeave(leave);
    cleanup(toLiveKitError(leave._reason));
    if (LeaveRequestAction::Disconnect == leave._action) {
        _client.resetParticipantSid();
    }
    else if (_reconnectAttempts < _options._reconnectAttempts) {
        // TODO: replace to media timer
        std::this_thread::sleep_for(_options._reconnectAttemptDelay);
        if (LeaveRequestAction::Resume == leave._action) {
            // should attempt a resume with `reconnect=1` in join URL
            _client.setParticipantSid(localParticipant().sid());
        }
        else {
            _client.resetParticipantSid();
        }
        // attempt to reconnect
        if (_client.connect()) {
            _reconnectAttempts.fetch_add(1U);
        }
        else if (canLogWarning()) {
            logWarning("Couldn't reconnect to server, attempt " +
                       std::to_string(_reconnectAttempts) + " of " +
                       std::to_string(_options._reconnectAttempts));
        }
    }
}

void RTCEngine::onTrackUnpublished(const TrackUnpublishedResponse& unpublished)
{
    RTCMediaEngine::onTrackUnpublished(unpublished);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        pcManager->removeTrack(localTrack(unpublished._trackSid, false));
    }
}

void RTCEngine::onTransportStateChanged(TransportState state)
{
    switch (state) {
        case TransportState::Connecting:
            _localDcs.setListener(this);
            _remoteDcs.setListener(this);
            break;
        case TransportState::Disconnected:
            cleanup();
            break;
        default:
            break;
    }
    changeState(state);
    if (TransportState::Connected == state) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            changeState(pcManager->state());
        }
    }
}

void RTCEngine::onTransportError(std::string error)
{
    logError(error);
    cleanup(LiveKitError::Transport, error);
}

bool RTCEngine::onPingRequested()
{
    Ping ping;
    const auto epochTime = std::chrono::system_clock::now().time_since_epoch();
    ping._timestamp = static_cast<int64_t>(epochTime / std::chrono::seconds(1));
    return _client.sendPingReq(ping);
}

void RTCEngine::onPongTimeout()
{
    logError("ping/pong timed out");
    cleanup(LiveKitError::ServerPingTimedOut);
}

void RTCEngine::onUserPacket(const std::string& participantSid,
                             const std::string& participantIdentity,
                             const std::string& payload,
                             const std::vector<std::string>& /*destinationIdentities*/,
                             const std::string& topic)
{
    callback(&SessionListener::onUserPacketReceived, participantSid,
             participantIdentity, payload, topic);
}

void RTCEngine::onChatMessage(const std::string& remoteParticipantIdentity,
                              const std::string& message, const std::string& id,
                              int64_t timestamp, bool deleted, bool generated)
{
    callback(&SessionListener::onChatMessageReceived, remoteParticipantIdentity,
             message, id, timestamp, deleted, generated);
}

std::string_view RTCEngine::logCategory() const
{
    static const std::string_view category("rtc_engine");
    return category;
}

} // namespace LiveKitCpp
#endif
