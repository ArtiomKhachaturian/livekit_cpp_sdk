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
#include "RTCEngineImpl.h"
#include "AesCgmCryptor.h"
#include "LocalParticipant.h"
#include "AudioDeviceImpl.h"
#include "LocalVideoDeviceImpl.h"
#include "LocalAudioTrackImpl.h"
#include "LocalVideoTrackImpl.h"
#include "PeerConnectionFactory.h"
#include "RemoteParticipantImpl.h"
#include "RoomUtils.h"
#include "RtcUtils.h"
#include "SignalTransportListenerAsync.h"
#include "TransportManager.h"
#include "WebsocketEndPoint.h"
#include "Utils.h"
#include "livekit/rtc/SessionListener.h"
#include "livekit/rtc/SessionListener.h"
#include "livekit/rtc/LiveKitError.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/e2e/KeyProviderOptions.h"
#include "livekit/signaling/sfu/UpdateLocalAudioTrack.h"
#include <algorithm> // for std::min
#include <thread>

namespace {

using namespace LiveKitCpp;

inline std::string formatErrorMsg(LiveKitError error, const std::string& what) {
    auto msg = toString(error);
    if (!msg.empty() && !what.empty()) {
        msg += ": " + what;
    }
    return msg;
}

inline constexpr bool failed(webrtc::PeerConnectionInterface::PeerConnectionState state) {
    return webrtc::PeerConnectionInterface::PeerConnectionState::kFailed == state;
}

}

namespace LiveKitCpp
{

enum class RTCEngineImpl::SendResult
{
    Ok,
    TransportError,
    TransportClosed
};

RTCEngineImpl::RTCEngineImpl(Options options, bool disableAudioRed,
                             PeerConnectionFactory* pcf,
                             const Participant* session,
                             std::unique_ptr<Websocket::EndPoint> socket,
                             const std::shared_ptr<Bricks::Logger>& logger)
    :  Bricks::LoggableS<ResponsesListener>(logger)
    , _localParticipant(new LocalParticipant(pcf, session, logger))
    , _remoteParicipants(new RemoteParticipants(options._autoSubscribe, this, logger))
    , _transportListener(std::make_unique<SignalTransportListenerAsync>(pcf))
    , _options(std::move(options))
    , _disableAudioRed(disableAudioRed)
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
    _client.setTransportListener(_transportListener.get());
    _transportListener->set(this);
}

RTCEngineImpl::~RTCEngineImpl()
{
    _localParticipant->reset();
    _transportListener->set(nullptr);
    _client.setServerListener(nullptr);
    _client.setTransportListener(nullptr);
    cleanup();
}

void RTCEngineImpl::setListener(SessionListener* listener)
{
    _listener = listener;
    _localParticipant->setListener(listener);
}

std::string RTCEngineImpl::addTrackDevice(std::unique_ptr<AudioDevice> device, EncryptionType encryption)
{
    if (device) {
        const auto id = device->id();
        if (auto impl = _localParticipant->addDevice(std::move(device), encryption)) {
            if (const auto pcManager = std::atomic_load(&_pcManager)) {
                pcManager->addTrack(std::move(impl), encryption);
            }
            return id;
        }
    }
    return {};
}

std::string RTCEngineImpl::addTrackDevice(std::unique_ptr<LocalVideoDevice> device, EncryptionType encryption)
{
    if (device) {
        const auto id = device->id();
        if (auto impl = _localParticipant->addDevice(std::move(device), encryption)) {
            if (const auto pcManager = std::atomic_load(&_pcManager)) {
                pcManager->addTrack(std::move(impl), encryption);
            }
            return id;
        }
    }
    return {};
}

void RTCEngineImpl::removeTrackDevice(const std::string& deviceId)
{
    if (_localParticipant->removeDevice(deviceId)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->removeTrack(deviceId);
        }
    }
}

void RTCEngineImpl::setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider)
{
    std::shared_ptr<KeyProvider> aesCgmKeyProvider;
    if (provider) {
        aesCgmKeyProvider.reset(provider.release());
        aesCgmKeyProvider->setSifTrailer(_sifTrailer());
    }
    std::atomic_store(&_aesCgmKeyProvider, std::move(aesCgmKeyProvider));
}

void RTCEngineImpl::setAudioPlayout(bool playout)
{
    if (exchangeVal(playout, _playout)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setAudioPlayout(playout);
        }
    }
}

void RTCEngineImpl::setAudioRecording(bool recording)
{
    if (exchangeVal(recording, _recording)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setAudioRecording(recording);
        }
    }
}

bool RTCEngineImpl::connect(std::string url, std::string authToken)
{
    if (url.empty()) {
        if (canLogError()) {
            logError("server URL is empty");
        }
        return false;
    }
    if (authToken.empty()) {
        if (canLogError()) {
            logError("authentification token is empty");
        }
        return false;
    }
    _client.setHost(std::move(url));
    _client.setAuthToken(std::move(authToken));
    return _client.connect();
}

void RTCEngineImpl::disconnect()
{
    if (auto pcManager = std::atomic_exchange(&_pcManager, std::shared_ptr<TransportManager>())) {
        // send leave event
        sendLeave();
        _client.disconnect();
        pcManager->close();
        pcManager->setListener(nullptr);
        _localDcs.setListener(nullptr);
        _remoteDcs.setListener(nullptr);
        notifyAboutLocalParticipantJoinLeave(false);
    }
}

bool RTCEngineImpl::sendUserPacket(std::string payload, bool reliable,
                                   const std::string& topic,
                                   const std::vector<std::string>& destinationSids,
                                   const std::vector<std::string>& destinationIdentities) const
{
    return _localDcs.sendUserPacket(std::move(payload), reliable, topic,
                                    destinationSids, destinationIdentities);
}

bool RTCEngineImpl::sendChatMessage(std::string message,
                                    bool deleted, bool generated,
                                    const std::vector<std::string>& destinationIdentities) const
{
    return _localDcs.sendChatMessage(std::move(message), deleted, generated, destinationIdentities);
}

void RTCEngineImpl::queryStats(const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(callback);
        }
    }
}

void RTCEngineImpl::cleanup(const std::optional<LiveKitError>& error, const std::string& errorDetails)
{
    _remoteParicipants->reset();
    disconnect();
    if (error) {
        if (canLogError()) {
            logError(formatErrorMsg(error.value(), errorDetails));
        }
        notify(&SessionListener::onError, error.value(), errorDetails);
    }
}

/*webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngineImpl::localTrack(const std::string& id,
                                                                                   bool cid) const
{
    if (const auto t = _localParticipant->track(id, cid)) {
        return t->media();
    }
    return {};
}*/

RTCEngineImpl::SendResult RTCEngineImpl::sendAddTrack(AddTrackRequest request) const
{
    return sendRequestToServer(&SignalClient::sendAddTrack, std::move(request));
}

RTCEngineImpl::SendResult RTCEngineImpl::sendMuteTrack(MuteTrackRequest request) const
{
    return sendRequestToServer(&SignalClient::sendMuteTrack, std::move(request));
}

RTCEngineImpl::SendResult RTCEngineImpl::sendUpdateLocalAudioTrack(UpdateLocalAudioTrack request) const
{
    return sendRequestToServer(&SignalClient::sendUpdateAudioTrack, std::move(request));
}

bool RTCEngineImpl::closed() const
{
    if (TransportState::Connected != _client.transportState()) {
        return true;
    }
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        return pcManager->closed();
    }
    return true;
}

template <class Method, typename... Args>
void RTCEngineImpl::notify(const Method& method, Args&&... args) const
{
    _listener.invoke(method, std::forward<Args>(args)...);
}

std::shared_ptr<ParticipantAccessor> RTCEngineImpl::participant(const std::string& sid) const
{
    if (sid == _localParticipant->sid()) {
        return _localParticipant;
    }
    return _remoteParicipants->at(sid);
}

void RTCEngineImpl::handleLocalParticipantDisconnection(DisconnectReason reason)
{
    if (DisconnectReason::UnknownReason != reason) {
        cleanup(toLiveKitError(reason));
    }
}

void RTCEngineImpl::notifyAboutLocalParticipantJoinLeave(bool join)
{
    if (!_localParticipant->sid().empty() && exchangeVal(join, _joined)) {
        if (join) {
            notify(&SessionListener::onLocalParticipantJoined);
        }
        else {
            notify(&SessionListener::onLocalParticipantLeaved);
        }
    }
}

template <class TTrack>
bool RTCEngineImpl::sendAddTrack(const std::shared_ptr<TTrack>& track)
{
    if (track && !closed()) {
        AddTrackRequest request;
        if (track->fillRequest(&request)) {
            switch (sendAddTrack(std::move(request))) {
                case SendResult::Ok:
                    if (canLogVerbose()) {
                        logVerbose("add local track '" + track->cid() + "' request has been sent to server");
                    }
                    return true;
                case SendResult::TransportError:
                    if (canLogError()) {
                        logError("failed to send add local track '" + track->cid() + "' request to server");
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return false;
}

void RTCEngineImpl::sendLeave(DisconnectReason reason, LeaveRequestAction action) const
{
    LeaveRequest request;
    request._reason = reason;
    request._action = action;
    _client.sendLeave(std::move(request));
}

template <class ReqMethod, class TReq>
RTCEngineImpl::SendResult RTCEngineImpl::sendRequestToServer(const ReqMethod& method, TReq req) const
{
    if (!closed()) {
        if ((_client.*method)(std::move(req))) {
            return SendResult::Ok;
        }
        return SendResult::TransportError;
    }
    return SendResult::TransportClosed;
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngineImpl::
    makeConfiguration(const std::vector<ICEServer>& iceServers, const std::optional<ClientConfiguration>& cc) const
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
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
    if (_options._audioJitterBufferFastAccelerate) {
        config.audio_jitter_buffer_fast_accelerate = _options._audioJitterBufferFastAccelerate.value();
    }
    if (_options._audioJitterBufferMinDelayMs) {
        config.audio_jitter_buffer_min_delay_ms = _options._audioJitterBufferMinDelayMs.value();
    }
    if (_options._audioJitterBufferMaxPackets) {
        config.audio_jitter_buffer_max_packets = _options._audioJitterBufferMaxPackets.value();
    }
    config.continual_gathering_policy = webrtc::PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
    return config;
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngineImpl::makeConfiguration(const JoinResponse& response) const
{
    return makeConfiguration(response._iceServers, response._clientConfiguration);
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngineImpl::makeConfiguration(const ReconnectResponse& response) const
{
    return makeConfiguration(response._iceServers, response._clientConfiguration);
}

void RTCEngineImpl::changeState(SessionState state)
{
    if (exchangeVal(state, _state)) {
        notify(&SessionListener::onStateChanged, state);
    }
}

void RTCEngineImpl::changeState(webrtc::PeerConnectionInterface::PeerConnectionState state)
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

void RTCEngineImpl::changeState(TransportState state)
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

void RTCEngineImpl::createTransportManager(const JoinResponse& response,
                                           const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
{
    const auto negotiationDelay = std::min<uint64_t>(_options._negotiationDelay.count(), 100ULL);
    auto pcManager = std::make_shared<TransportManager>(response._subscriberPrimary,
                                                        response._fastPublish,
                                                        _disableAudioRed,
                                                        response._pingTimeout,
                                                        response._pingInterval,
                                                        negotiationDelay,
                                                        response._participant._tracks,
                                                        _pcf, conf,
                                                        weak_from_this(),
                                                        response._participant._identity,
                                                        _options._prefferedAudioEncoder,
                                                        _options._prefferedVideoEncoder,
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
    _localParticipant->addDevicesToTransportManager(pcManager.get());
    pcManager->negotiate(false);
    pcManager->startPing();
}

void RTCEngineImpl::queryStats(const webrtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                               const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (receiver && callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(receiver, callback);
        }
    }
}

void RTCEngineImpl::queryStats(const webrtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                               const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (sender && callback) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->queryStats(sender, callback);
        }
    }
}

webrtc::scoped_refptr<webrtc::FrameTransformerInterface> RTCEngineImpl::
    createCryptor(EncryptionType encryption, webrtc::MediaType mediaType,
                  std::string identity, std::string trackId,
                  const std::weak_ptr<AesCgmCryptorObserver>& observer) const
{
    if (_pcf && EncryptionType::Gcm == encryption) {
        if (const auto provider = std::atomic_load(&_aesCgmKeyProvider)) {
            auto cryptor = AesCgmCryptor::create(mediaType, std::move(identity),
                                                 std::move(trackId),
                                                 _pcf->eventsQueue(),
                                                 provider, logger());
            if (cryptor) {
                cryptor->setObserver(observer);
                return cryptor;
            }
        }
    }
    return {};
}

void RTCEngineImpl::onUpdateSubscription(UpdateSubscription subscription)
{
    sendRequestToServer(&SignalClient::sendSubscription, std::move(subscription));
}

void RTCEngineImpl::notifyAboutMuteChanges(const std::string& trackSid, bool muted)
{
    if (!trackSid.empty()) {
        MuteTrackRequest request;
        request._sid = trackSid;
        request._muted = muted;
        const auto result = sendMuteTrack(std::move(request));
        if (SendResult::TransportError == result) {
            // TODO: log error
        }
    }
}

void RTCEngineImpl::notifyAboutSetRtpParametersFailure(const std::string& trackSid,
                                                       std::string_view details)
{
    if (!trackSid.empty() && canLogError()) {
        std::vector<std::string_view> errors;
        errors.push_back("failed to set RTP parameters for track '" + trackSid + "'");
        errors.push_back(std::move(details));
        logError(join(errors, ": "));
    }
}

std::optional<bool> RTCEngineImpl::stereoRecording() const
{
    return _localParticipant->stereoRecording();
}

void RTCEngineImpl::onParticipantAdded(const std::string& sid)
{
    notify(&SessionListener::onRemoteParticipantAdded, sid);
}

void RTCEngineImpl::onParticipantRemoved(const std::string& sid)
{
    notify(&SessionListener::onRemoteParticipantRemoved, sid);
}

void RTCEngineImpl::onJoin(JoinResponse response)
{
    const auto disconnectReason = response._participant._disconnectReason;
    if (DisconnectReason::UnknownReason == disconnectReason) {
        _sifTrailer(response._sifTrailer);
        if (const auto provider = std::atomic_load(&_aesCgmKeyProvider)) {
            provider->setSifTrailer(response._sifTrailer);
        }
        _localParticipant->setInfo(response._participant);
        _remoteParicipants->setInfo(weak_from_this(), response._otherParticipants);
        _localDcs.setSid(response._participant._sid);
        _localDcs.setIdentity(response._participant._identity);
        createTransportManager(response, makeConfiguration(response));
        _lastJoinResponse(std::move(response));
        notifyAboutLocalParticipantJoinLeave(true);
    }
    else {
        handleLocalParticipantDisconnection(disconnectReason);
    }
}

void RTCEngineImpl::onUpdate(ParticipantUpdate update)
{
    std::vector<ParticipantInfo> infos = update._participants;
    if (const auto s = infos.size()) {
        DisconnectReason disconnectReason = DisconnectReason::UnknownReason;
        for (size_t i = 0U; i < s; ++i) {
            auto& info = infos.at(i);
            if (info._sid == _localParticipant->sid()) {
                disconnectReason = info._disconnectReason;
                if (DisconnectReason::UnknownReason == disconnectReason) {
                    _localParticipant->setInfo(info);
                    if (const auto pcManager = std::atomic_load(&_pcManager)) {
                        pcManager->updateTracksInfo(std::move(info._tracks));
                    }
                    infos.erase(infos.begin() + i);
                }
                break;
            }
        }
        if (DisconnectReason::UnknownReason == disconnectReason) {
            _remoteParicipants->updateInfo(weak_from_this(), infos);
        }
        else {
            handleLocalParticipantDisconnection(disconnectReason);
        }
    }
}

void RTCEngineImpl::onTrackPublished(TrackPublishedResponse published)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        std::optional<webrtc::MediaType> hint;
        switch (published._track._type) {
            case TrackType::Audio:
                hint = webrtc::MediaType::AUDIO;
                break;
            case TrackType::Video:
                hint = webrtc::MediaType::VIDEO;
                break;
            default:
                break;
        }
        if (const auto t = pcManager->track(published._cid, true, hint)) {
            t->setSid(published._track._sid);
            // reconcile track mute status.
            // if server's track mute status doesn't match actual, we'll have to update
            // the server's copy
            const auto muted = t->muted();
            if (muted != published._track._muted) {
                notifyAboutMuteChanges(published._track._sid, muted);
            }
            switch (published._track._type) {
                case TrackType::Audio:
                    if (const auto audio = std::dynamic_pointer_cast<AudioTrack>(t)) {
                        auto features = audio->features();
                        if (!features.empty()) {
                            UpdateLocalAudioTrack request;
                            request._trackSid = published._track._sid;
                            request._features = std::move(features);
                            sendUpdateLocalAudioTrack(std::move(request));
                        }
                    }
                    break;
                case TrackType::Video:
                    break;
                default:
                    break;
            }
        }
    }
}

void RTCEngineImpl::onReconnect(ReconnectResponse response)
{
    notifyAboutLocalParticipantJoinLeave(true);
    createTransportManager(_lastJoinResponse(), makeConfiguration(response));
}

void RTCEngineImpl::onMute(MuteTrackRequest mute)
{
    bool accepted = false;
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        accepted = pcManager->setRemoteSideTrackMute(mute._sid, mute._muted);
    }
    if (!accepted) {
        _remoteParicipants->setRemoteSideTrackMute(mute._sid, mute._muted);
    }
}

void RTCEngineImpl::onSpeakersChanged(SpeakersChanged changed)
{
    for (const auto& speakerInfo : changed._speakers) {
        if (const auto p = participant(speakerInfo._sid)) {
            p->setSpeakerChanges(speakerInfo._level, speakerInfo._active);
        }
    }
}

void RTCEngineImpl::onConnectionQuality(ConnectionQualityUpdate update)
{
    for (const auto& updateInfo : update._updates) {
        if (const auto p = participant(updateInfo._participantSid)) {
            p->setConnectionQuality(updateInfo._quality, updateInfo._score);
        }
    }
}

void RTCEngineImpl::onOffer(SessionDescription sdp)
{
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setRemoteOffer(std::move(desc));
        }
    }
    else if (canLogError()) {
        logError("failed to parse remote offer SDP: " + error.description);
    }
}

void RTCEngineImpl::onAnswer(SessionDescription sdp)
{
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setRemoteAnswer(std::move(desc));
        }
    }
    else if (canLogError()) {
        logError("failed to parse remote answer SDP: " + error.description);
    }
}

void RTCEngineImpl::onPong(Pong pong)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        // Clear timeout timer
        pcManager->notifyThatPongReceived();
    }
}

void RTCEngineImpl::onTrickle(TrickleRequest request)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        webrtc::SdpParseError error;
        if (auto candidate = RoomUtils::map(request._candidate, &error)) {
            pcManager->addIceCandidate(request._target, std::move(candidate));
        }
        else if (canLogError()) {
            logError("failed to parse ICE candidate SDP for " +
                     toString(request._target) + ": " + error.description);
        }
    }
}

void RTCEngineImpl::onLeave(LeaveRequest leave)
{
    cleanup(toLiveKitError(leave._reason));
    if (LeaveRequestAction::Disconnect == leave._action) {
        _client.resetParticipantSid();
    }
    else if (_reconnectAttempts < _options._reconnectAttempts) {
        // TODO: replace to media timer
        std::this_thread::sleep_for(_options._reconnectAttemptDelay);
        if (LeaveRequestAction::Resume == leave._action) {
            // should attempt a resume with `reconnect=1` in join URL
            _client.setParticipantSid(_localParticipant->sid());
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

void RTCEngineImpl::onTrackUnpublished(TrackUnpublishedResponse unpublished)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        pcManager->removeTrack(unpublished._trackSid, false);
    }
}

void RTCEngineImpl::onRefreshToken(std::string authToken)
{
    notify(&SessionListener::onRefreshToken, std::move(authToken));
}

void RTCEngineImpl::onLocalAudioTrackAdded(const std::shared_ptr<LocalAudioTrackImpl>& track)
{
    if (track) {
        if (EncryptionType::None != track->encryption()) {
            if (auto cryptor = createCryptor(track->encryption(), track->mediaType(),
                                             _localParticipant->identity(),
                                             track->id(), _localParticipant)) {
                track->setFrameTransformer(std::move(cryptor));
            }
            else if (canLogError())  {
                logError("failed to create " + toString(track->encryption()) + " encryptor for track " + track->cid());
            }
        }
        sendAddTrack(track);
        _listener.invoke(&SessionListener::onLocalAudioTrackAdded, track);
    }
}

void RTCEngineImpl::onLocalVideoTrackAdded(const std::shared_ptr<LocalVideoTrackImpl>& track)
{
    if (track) {
        if (EncryptionType::None != track->encryption()) {
            if (auto cryptor = createCryptor(track->encryption(), track->mediaType(),
                                             _localParticipant->identity(),
                                             track->id(), _localParticipant)) {
                track->setFrameTransformer(std::move(cryptor));
            }
            else if (canLogError())  {
                logError("failed to create " + toString(track->encryption()) + " encryptor for track " + track->cid());
            }
        }
        sendAddTrack(track);
        _listener.invoke(&SessionListener::onLocalVideoTrackAdded, track);
    }
}

void RTCEngineImpl::onLocalTrackAddFailure(std::string id, webrtc::MediaType type, webrtc::RTCError error)
{
    switch (type) {
        case webrtc::MediaType::AUDIO:
            _listener.invoke(&SessionListener::onLocalAudioTrackAddFailure, std::move(id), error.message());
            break;
        case webrtc::MediaType::VIDEO:
            _listener.invoke(&SessionListener::onLocalVideoTrackAddFailure, std::move(id), error.message());
            break;
        default:
            break;
    }
}

void RTCEngineImpl::onLocalTrackRemoved(std::string id, webrtc::MediaType type)
{
    switch (type) {
        case webrtc::MediaType::AUDIO:
            _listener.invoke(&SessionListener::onLocalAudioTrackRemoved, std::move(id));
            break;
        case webrtc::MediaType::VIDEO:
            _listener.invoke(&SessionListener::onLocalVideoTrackRemoved, std::move(id));
            break;
        default:
            break;
    }
}

void RTCEngineImpl::onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState state,
                                  webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                                  webrtc::PeerConnectionInterface::PeerConnectionState subscriberState)
{
    switch (subscriberState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            _remoteParicipants->reset();
            break;
        default:
            break;
    }
    if (failed(state) || failed(publisherState) || failed(subscriberState)) {
        cleanup(LiveKitError::RTC);
    }
    else {
        changeState(state);
    }
}

void RTCEngineImpl::onRemoteTrackAdded(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                       std::string trackId, std::string participantSid)
{
    _remoteParicipants->addMedia(weak_from_this(), receiver, std::move(trackId), std::move(participantSid));
}

void RTCEngineImpl::onRemotedTrackRemoved(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    _remoteParicipants->removeMedia(receiver);
}

void RTCEngineImpl::onSdpOperationFailed(SignalTarget, webrtc::RTCError error)
{
    sendLeave();
    cleanup(LiveKitError::RTC, error.message());
}

void RTCEngineImpl::onNegotiationNeeded()
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        pcManager->negotiate(true);
    }
}

void RTCEngineImpl::onPublisherOffer(std::string type, std::string sdp)
{
    if (auto offer = RoomUtils::map(std::move(type), std::move(sdp))) {
        switch (sendRequestToServer(&SignalClient::sendOffer, std::move(offer.value()))) {
            case SendResult::Ok:
                if (canLogInfo()) {
                    logInfo("publisher offer has been sent to server");
                }
                break;
            case SendResult::TransportError:
                if (canLogError()) {
                    logError("failed to send publisher offer to the server - transport error");
                }
                break;
            case SendResult::TransportClosed:
                if (canLogWarning()) {
                    logWarning("failed to send publisher offer to the server - transport is already closed");
                }
                break;
        }
    }
    else if (canLogError()) {
        logError("failed to serialize publisher offer into a string");
    }
}

void RTCEngineImpl::onSubscriberAnswer(std::string type, std::string sdp)
{
    if (auto answer = RoomUtils::map(std::move(type), std::move(sdp))) {
        switch (sendRequestToServer(&SignalClient::sendAnswer, std::move(answer.value()))) {
            case SendResult::Ok:
                if (canLogInfo()) {
                    logInfo("publisher answer has been sent to server");
                }
                break;
            case SendResult::TransportError:
                if (canLogError()) {
                    logError("failed to send publisher answer to the server - transport error");
                }
                break;
            case SendResult::TransportClosed:
                if (canLogWarning()) {
                    logWarning("failed to send publisher answer to the server - transport is already closed");
                }
                break;
        }
    }
    else if (canLogError()) {
        logError("failed to serialize publisher answer into a string");
    }
}

void RTCEngineImpl::onIceCandidateGathered(SignalTarget target, std::string sdpMid,
                                           int sdpMlineIndex, webrtc::Candidate candidate)
{
    TrickleRequest request;
    request._candidate = RoomUtils::map(std::move(sdpMid), sdpMlineIndex, candidate);
    if (request._candidate) {
        request._target = target;
        request._final = false;
        if (!_client.sendTrickle(std::move(request)) && canLogError()) {
            logError("failed to send " + candidate.url() +
                     " " + toString(target) + " local ICE candidate");
        }
    }
    else if (canLogError()) {
        logError("failed to serialize " + candidate.url() +
                 " " + toString(target) + " local ICE candidate");
    }
}

void RTCEngineImpl::onLocalDataChannelCreated(webrtc::scoped_refptr<DataChannel> channel)
{
    _localDcs.add(std::move(channel));
}

void RTCEngineImpl::onRemoteDataChannelOpened(webrtc::scoped_refptr<DataChannel> channel)
{
    _remoteDcs.add(std::move(channel));
}

void RTCEngineImpl::onTransportStateChanged(TransportState state)
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

void RTCEngineImpl::onTransportError(std::string error)
{
    if (canLogError()) {
        logError(error);
    }
    cleanup(LiveKitError::Transport, error);
}

bool RTCEngineImpl::onPingRequested()
{
    Ping ping;
    const auto epochTime = std::chrono::system_clock::now().time_since_epoch();
    ping._timestamp = static_cast<int64_t>(epochTime / std::chrono::seconds(1));
    return SendResult::Ok == sendRequestToServer(&SignalClient::sendPingReq, std::move(ping));
}

void RTCEngineImpl::onPongTimeout()
{
    if (canLogError()) {
        logError("ping/pong timed out");
    }
    cleanup(LiveKitError::ServerPingTimedOut);
}

void RTCEngineImpl::onUserPacket(UserPacket packet, std::string participantIdentity,
                                 std::vector<std::string> destinationIdentities)
{
    notify(&SessionListener::onUserPacketReceived, packet,
           participantIdentity, destinationIdentities);
}

void RTCEngineImpl::onChatMessage(ChatMessage message, std::string participantIdentity,
                                  std::vector<std::string> destinationIdentities)
{
    notify(&SessionListener::onChatMessageReceived, message,
           participantIdentity, destinationIdentities);
}

std::string_view RTCEngineImpl::logCategory() const
{
    static const std::string_view category("rtc_engine");
    return category;
}

} // namespace LiveKitCpp
