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
#include "TransportManager.h"
#include "PeerConnectionFactory.h"
#include "WebsocketEndPoint.h"
#include "RoomUtils.h"
#include "./Camera/CameraVideoTrack.h"
#include "./Camera/CameraVideoSource.h"
#include "./Camera/CameraManager.h"
#include "rtc/SignalTarget.h"
#include "rtc/ReconnectResponse.h"
#include "rtc/Ping.h"
#include "rtc/TrickleRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/LeaveRequest.h"
#include <modules/video_capture/video_capture_factory.h>
#include <thread>

namespace LiveKitCpp
{

RTCEngine::RTCEngine(const Options& signalOptions,
                     PeerConnectionFactory* pcf,
                     std::unique_ptr<Websocket::EndPoint> socket,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : RTCMediaEngine(logger)
    , _options(signalOptions)
    , _pcf(pcf)
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
    bool ok = false;
    if (!url.empty() && !authToken.empty()) {
        _joinAttempts.fetch_add(1U);
        _client.setHost(std::move(url));
        _client.setAuthToken(std::move(authToken));
        ok = _client.connect();
        if (!ok) {
            if (_joinAttempts < _options._reconnectAttempts) {
                if (canLogWarning()) {
                    logWarning("Couldn't connect to server, attempt " +
                               std::to_string(_joinAttempts) + " of " +
                               std::to_string(_options._reconnectAttempts));
                }
                // TODO: replace to media timer
                std::this_thread::sleep_for(_options._reconnectAttemptDelay);
                ok = connect(_client.host(), _client.authToken());
            }
        }
    }
    return ok;
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

void RTCEngine::cleanup(bool /*error*/)
{
    std::atomic_store(&_pcManager, std::shared_ptr<TransportManager>());
    _client.disconnect();
    cleanupLocalResources();
    cleanupRemoteResources();
}

bool RTCEngine::addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        return pcManager->addTrack(track);
    }
    return RTCMediaEngine::addLocalMedia(track);
}

bool RTCEngine::removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        return pcManager->removeTrack(track);
    }
    return RTCMediaEngine::removeLocalMedia(track);
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> RTCEngine::createMic(const std::string& label)
{
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> track;
    if (_pcf) {
        cricket::AudioOptions options; // TODO: should be a part of room config
        logInfo("request to create '" + label + "' audio track (" + options.ToString() + ")");
        if (const auto source = _pcf->CreateAudioSource(options)) {
            track = _pcf->CreateAudioTrack(label, source.get());
            if (track) {
                logVerbose("audio track '" + label + "' has been created");
            }
            else {
                logError("unable to create audio track '" + label + "'");
            }
        }
        else {
            logError("unable to create source for audio track '" + label + "'");
        }
    }
    return track;
}

webrtc::scoped_refptr<CameraVideoTrack> RTCEngine::createCamera(const std::string& label)
{
    webrtc::scoped_refptr<CameraVideoTrack> track;
    if (_pcf) {
        if (CameraManager::available()) {
            logInfo("request to create '" + label + "' video track");
            auto source = webrtc::make_ref_counted<CameraVideoSource>();
            track = webrtc::make_ref_counted<CameraVideoTrack>(label, std::move(source));
            logVerbose("video track '" + label + "' has been created");
        }
        else {
            logError("unable to create video track '" + label + "': camera manager is not available");
        }
    }
    return track;
}

RTCEngine::SendResult RTCEngine::sendAddTrack(const AddTrackRequest& request) const
{
    if (!closed()) {
        if (_client.sendAddTrack(request)) {
            return SendResult::Ok;
        }
        return SendResult::TransportError;
    }
    return SendResult::TransportClosed;
}

RTCEngine::SendResult RTCEngine::sendMuteTrack(const MuteTrackRequest& request) const
{
    if (!closed()) {
        if (_client.sendMuteTrack(request)) {
            return SendResult::Ok;
        }
        return SendResult::TransportError;
    }
    return SendResult::TransportClosed;
}

bool RTCEngine::sendLeave(DisconnectReason reason, LeaveRequestAction action) const
{
    LeaveRequest request;
    request._reason = reason;
    request._action = action;
    return _client.sendLeave(request);
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

void RTCEngine::onNegotiationNeeded()
{
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
        if (RoomUtils::map(candidate, request._candidateInit)) {
            request._target = target;
            request._final = false;
            if (!_client.sendTrickle(request)) {
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

void RTCEngine::onJoin(const JoinResponse& response)
{
    RTCMediaEngine::onJoin(response);
    TransportManagerListener* listener = this;
    auto pcManager = std::make_shared<TransportManager>(response, listener,
                                                        _pcf, makeConfiguration(response),
                                                        logger());
    std::atomic_store(&_pcManager, pcManager);
    for (auto media : pendingLocalMedia()) {
        pcManager->addTrack(std::move(media));
    }
    pcManager->negotiate(false);
    pcManager->startPing();
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
            if (!pcManager->addIceCandidate(request._target, std::move(candidate))) {
                // TODO: log error
            }
        }
        else {
            logError("failed to parse ICE candidate SDP for " +
                     toString(request._target) + ": " + error.description);
        }
    }
}

void RTCEngine::onTransportStateChanged(TransportState state)
{
    switch (state) {
        case TransportState::Connecting:
            addLocalResourcesToTransport();
            break;
        case TransportState::Disconnected:
            cleanup(false);
            break;
        default:
            break;
    }
}

void RTCEngine::onTransportError(std::string error)
{
    logError(error);
    cleanup(true);
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
    cleanup(true);
    //await self.cleanUp(withError: LiveKitError(.serverPingTimedOut))
}

std::string_view RTCEngine::logCategory() const
{
    static const std::string_view category("rtc_engine");
    //LOCK_READ_SAFE_OBJ(_latestJoinResponse);
    /*if (const auto& resp = _latestJoinResponse->value()) {
        
    }*/
    return category;
}

} // namespace LiveKitCpp
