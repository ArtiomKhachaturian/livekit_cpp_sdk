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
#include "RTCEngine.h"
#include "LocalParticipant.h"
#include "TransportManager.h"
#include "PeerConnectionFactory.h"
#include "WebsocketEndPoint.h"
#include "RoomUtils.h"
#include "Utils.h"
#include "livekit/rtc/SessionListener.h"
#include "livekit/signaling/NetworkType.h"
#include <algorithm> // for std::min
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
    resetLocalParticipant();
    _client.setServerListener(nullptr);
    _client.setTransportListener(nullptr);
    cleanup();
}

void RTCEngine::setAudioPlayout(bool playout)
{
    if (playout != _playout.exchange(playout)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setAudioPlayout(playout);
        }
    }
}

void RTCEngine::setAudioRecording(bool recording)
{
    if (recording != _recording.exchange(recording)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setAudioRecording(recording);
        }
    }
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
        // send leave event
        sendLeave();
        _client.disconnect();
        pcManager->close();
        pcManager->setListener(nullptr);
    }
}

bool RTCEngine::sendUserPacket(std::string payload, bool reliable,
                               const std::string& topic,
                               const std::vector<std::string>& destinationSids,
                               const std::vector<std::string>& destinationIdentities) const
{
    return _localDcs.sendUserPacket(std::move(payload), reliable, topic,
                                    destinationSids, destinationIdentities);
}

bool RTCEngine::sendChatMessage(std::string message,
                                bool deleted,
                                bool generated,
                                const std::vector<std::string>& destinationIdentities) const
{
    return _localDcs.sendChatMessage(std::move(message), deleted, generated, destinationIdentities);
}

void RTCEngine::queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(callback);
        }
    }
}

std::shared_ptr<LocalAudioTrackImpl> RTCEngine::addLocalAudioTrack(std::shared_ptr<AudioDevice> device)
{
    auto track = RTCMediaEngine::addLocalAudioTrack(std::move(device));
    if (track) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->addTrack(track->media());
        }
    }
    return track;
}

std::shared_ptr<CameraTrackImpl> RTCEngine::addLocalCameraTrack(std::shared_ptr<CameraDevice> device)
{
    auto track = RTCMediaEngine::addLocalCameraTrack(std::move(device));
    if (track) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->addTrack(track->media());
        }
    }
    return track;
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngine::
    removeLocalAudioTrack(std::shared_ptr<AudioTrack> track)
{
    auto media = RTCMediaEngine::removeLocalAudioTrack(std::move(track));
    if (media) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->removeTrack(media);
        }
    }
    return media;
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngine::
    removeLocalVideoTrack(std::shared_ptr<VideoTrack> track)
{
    auto media = RTCMediaEngine::removeLocalVideoTrack(std::move(track));
    if (media) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->removeTrack(media);
        }
    }
    return media;
}

void RTCEngine::queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                           const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (receiver && callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(receiver, callback);
        }
    }
}

void RTCEngine::queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                           const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (sender && callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(sender, callback);
        }
    }
}

RTCEngine::SendResult RTCEngine::sendAddTrack(AddTrackRequest request) const
{
    return sendRequestToServer(&SignalClient::sendAddTrack, std::move(request));
}

RTCEngine::SendResult RTCEngine::sendMuteTrack(MuteTrackRequest request) const
{
    return sendRequestToServer(&SignalClient::sendMuteTrack, std::move(request));
}

RTCEngine::SendResult RTCEngine::sendUpdateLocalAudioTrack(UpdateLocalAudioTrack request) const
{
    return sendRequestToServer(&SignalClient::sendUpdateAudioTrack, std::move(request));
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
        invoke(&SessionListener::onError, error.value(), errorDetails);
    }
}

void RTCEngine::sendLeave(DisconnectReason reason, LeaveRequestAction action) const
{
    LeaveRequest request;
    request._reason = reason;
    request._action = action;
    sendRequestToServer(&SignalClient::sendLeave, std::move(request));
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
RTCEngine::SendResult RTCEngine::sendRequestToServer(const ReqMethod& method, TReq req) const
{
    if (!closed()) {
        if ((_client.*method)(std::move(req))) {
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
    switch (activeNetworkType()) {
        case NetworkType::WiFi:
            config.network_preference = rtc::AdapterType::ADAPTER_TYPE_WIFI;
            break;
        case NetworkType::Wired:
            config.network_preference = rtc::AdapterType::ADAPTER_TYPE_ETHERNET;
            break;
        case NetworkType::Cellular:
            config.network_preference = rtc::AdapterType::ADAPTER_TYPE_CELLULAR;
            break;
        case NetworkType::Vpn:
            config.network_preference = rtc::AdapterType::ADAPTER_TYPE_VPN;
            break;
        default:
            break;
    }
    // enable ICE renomination, like on Android (   "a=ice-options:trickle renomination")
    //config.enable_ice_renomination = true;
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
        invoke(&SessionListener::onStateChanged, state);
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
    const auto negotiationDelay = std::min<uint64_t>(_options._negotiationDelay.count(), 100ULL);
    auto pcManager = std::make_shared<TransportManager>(response._subscriberPrimary,
                                                        response._fastPublish,
                                                        response._pingTimeout,
                                                        response._pingInterval,
                                                        negotiationDelay,
                                                        _pcf, conf,
                                                        response._participant._identity,
                                                        logger());
    pcManager->setListener(this);
    // both playout & recording are enabled by default
    if (!audioPlayoutEnabled()) {
        pcManager->setAudioPlayout(false);
    }
    if (!audioRecordingEnabled()) {
        pcManager->setAudioRecording(false);
    }
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
    if (auto sdp = RoomUtils::map(desc)) {
        if (!_client.sendOffer(std::move(sdp.value()))) {
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
    if (auto sdp = RoomUtils::map(desc)) {
        if (!_client.sendAnswer(std::move(sdp.value()))) {
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
        TrickleRequest request;
        request._candidate = RoomUtils::map(candidate);
        if (request._candidate) {
            request._target = target;
            request._final = false;
            if (!_client.sendTrickle(std::move(request))) {
                logError("failed to send " + candidate->server_url() +
                         " " + toString(target) + " local ICE candidate");
            }
        }
        else {
            logError("failed to serialize " + candidate->server_url() +
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

void RTCEngine::onJoin(JoinResponse response)
{
    RTCMediaEngine::onJoin(response);
    if (DisconnectReason::UnknownReason == response._participant._disconnectReason) {
        _localDcs.setSid(response._participant._sid);
        _localDcs.setIdentity(response._participant._identity);
        _lastJoinResponse(response);
        createTransportManager(response, makeConfiguration(response));
    }
}

void RTCEngine::onReconnect(ReconnectResponse response)
{
    RTCMediaEngine::onReconnect(response);
    createTransportManager(_lastJoinResponse(), makeConfiguration(response));
}

void RTCEngine::onOffer(SessionDescription sdp)
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

void RTCEngine::onAnswer(SessionDescription sdp)
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

void RTCEngine::onPong(Pong pong)
{
    RTCMediaEngine::onPong(pong);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        // Clear timeout timer
        pcManager->notifyThatPongReceived();
    }
}

void RTCEngine::onTrickle(TrickleRequest request)
{
    RTCMediaEngine::onTrickle(request);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        webrtc::SdpParseError error;
        if (auto candidate = RoomUtils::map(request._candidate, &error)) {
            pcManager->addIceCandidate(request._target, std::move(candidate));
        }
        else {
            logError("failed to parse ICE candidate SDP for " +
                     toString(request._target) + ": " + error.description);
        }
    }
}

void RTCEngine::onLeave(LeaveRequest leave)
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

void RTCEngine::onTrackUnpublished(TrackUnpublishedResponse unpublished)
{
    RTCMediaEngine::onTrackUnpublished(unpublished);
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        pcManager->removeTrack(localTrack(unpublished._trackSid, false));
    }
}

void RTCEngine::onRefreshToken(std::string authToken)
{
    invoke(&SessionListener::onRefreshToken, std::move(authToken));
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
    return _client.sendPingReq(std::move(ping));
}

void RTCEngine::onPongTimeout()
{
    logError("ping/pong timed out");
    cleanup(LiveKitError::ServerPingTimedOut);
}

void RTCEngine::onUserPacket(UserPacket packet,
                             std::string participantIdentity,
                             std::vector<std::string> destinationIdentities)
{
    invoke(&SessionListener::onUserPacketReceived, packet,
           participantIdentity, destinationIdentities);
}

void RTCEngine::onChatMessage(ChatMessage message,
                              std::string participantIdentity,
                              std::vector<std::string> destinationIdentities)
{
    invoke(&SessionListener::onChatMessageReceived, message,
           participantIdentity, destinationIdentities);
}

std::string_view RTCEngine::logCategory() const
{
    static const std::string_view category("rtc_engine");
    return category;
}

} // namespace LiveKitCpp
