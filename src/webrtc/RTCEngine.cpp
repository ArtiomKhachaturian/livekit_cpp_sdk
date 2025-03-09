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
#include "rtc/SignalTarget.h"
#include "rtc/ReconnectResponse.h"
#include "rtc/Ping.h"
#include "rtc/TrickleRequest.h"
#include <thread>

namespace LiveKitCpp
{

RTCEngine::RTCEngine(const Options& signalOptions,
                     PeerConnectionFactory* pcf,
                     std::unique_ptr<Websocket::EndPoint> socket,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportManagerListener,
                        SignalTransportListener,
                        SignalServerListener,
                        MediaTimerCallback>(logger)
    , _options(signalOptions)
    , _pcf(pcf)
    , _localAudioTrack(createLocalAudioTrack())
    , _client(std::move(socket), logger.get())
    , _pingIntervalTimer(_pcf, this, logger, "ping_interval_timer")
    , _pingTimeoutTimer(_pcf, this, logger, "ping_timeout_timer")
{
    _client.setAdaptiveStream(_options._adaptiveStream);
    _client.setAutoSubscribe(_options._autoSubscribe);
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
                std::this_thread::sleep_for(_options._reconnectAttemptDelay);
                ok = connect(_client.host(), _client.authToken());
            }
        }
    }
    return ok;
}

void RTCEngine::restartPingTimer()
{
    // Always cancel first...
    _pingTimeoutTimer.stop();
    _pingIntervalTimer.stop();
    // Check previously received joinResponse
    const auto ljr = std::atomic_load(&_latestJoinResponse);
    if (ljr && ljr->_pingTimeout > 0 && ljr->_pingInterval > 0) {
        if (canLogVerbose()) {
            logVerbose("ping/pong starting with interval: " +
                       std::to_string(ljr->_pingInterval) +
                       "s, timeout: " + std::to_string(ljr->_pingTimeout) + "s");
        }
        // Update interval...
        _pingIntervalTimer.start(ljr->_pingInterval * 1000);
    }
}

bool RTCEngine::sendPing()
{
    Ping ping;
    const auto epochTime = std::chrono::system_clock::now().time_since_epoch();
    ping._timestamp = static_cast<int64_t>(epochTime / std::chrono::seconds(1));
    const auto done = _client.sendPingReq(ping);
    if (done) {
        logVerbose("ping/pong sending ping...");
    }
    else {
        logError("ping/pong sending failed");
    }
    return done;
}

void RTCEngine::cleanup(bool /*error*/)
{
    std::atomic_store(&_latestJoinResponse, std::shared_ptr<const JoinResponse>());
    std::atomic_store(&_pcManager, std::shared_ptr<TransportManager>());
    _pingTimeoutTimer.stop();
    _pingTimeoutTimer.stop();
    _client.disconnect();
}

rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCEngine::createLocalAudioTrack() const
{
    if (_pcf) {
        auto source = _pcf->CreateAudioSource({});
        if (source) {
            return _pcf->CreateAudioTrack("mic", source.get());
        }
    }
    return {};
}

webrtc::PeerConnectionInterface::RTCConfiguration RTCEngine::
    makeConfiguration(const std::vector<ICEServer>& iceServers,
                      const std::optional<ClientConfiguration>& cc) const
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    //config.network_preference.emplace(::rtc::AdapterType::ADAPTER_TYPE_ETHERNET); // ethernet is preferred
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

void RTCEngine::onPublisherOffer(const webrtc::SessionDescriptionInterface* desc)
{
    if (const auto sdp = RoomUtils::map(desc)) {
        if (_client.sendOffer(sdp.value())) {
            logInfo("publisher offer already sent to the server");
        }
        else {
            logError("failed to send publisher offer to the server");
        }
    }
    else {
        logError("failed to serialize publisher offer into a string");
    }
}

void RTCEngine::onSubscriberAnswer(const webrtc::SessionDescriptionInterface* desc)
{
    if (const auto sdp = RoomUtils::map(desc)) {
        if (_client.sendAnswer(sdp.value())) {
            logInfo("subscriber answer already sent to the server");
        }
        else {
            logError("failed to send subscriber answer to the server");
        }
    }
    else {
        logError("failed to serialize subscriber answer into a string");
    }
}

void RTCEngine::onIceCandidate(SignalTarget target,
                               const webrtc::IceCandidateInterface* candidate)
{
    if (candidate) {
        TrickleRequest request;
        if (RoomUtils::map(candidate, request._candidateInit)) {
            request._target = target;
            request._final = false;
            if (!_client.sendTrickle(request)) {
                logError("failed to send " + candidate->server_url() +
                         " " + std::string(toString(target)) + " local ICE candidate");
            }
        }
        else {
            logError("failed to serialize " + candidate->server_url() +
                     " " + std::string(toString(target)) + " local ICE candidate");
        }
    }
}

void RTCEngine::onJoin(uint64_t, const JoinResponse& response)
{
    logVerbose("join response received from server");
    std::atomic_store(&_latestJoinResponse, std::make_shared<const JoinResponse>(response));
    TransportManagerListener* listener = this;
    auto pcManager = std::make_shared<TransportManager>(response._subscriberPrimary,
                                                        listener, _pcf,
                                                        makeConfiguration(response),
                                                        logger());
    std::atomic_store(&_pcManager, pcManager);
    pcManager->createPublisherOffer();
    restartPingTimer();
}

void RTCEngine::onOffer(uint64_t, const SessionDescription& sdp)
{
    logVerbose("remote offer SDP received from server");
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setSubscriberRemoteOffer(std::move(desc));
        }
    }
    else  {
        logError("failed to parse remote offer for subscriber: " + error.description);
    }
}

void RTCEngine::onAnswer(uint64_t, const SessionDescription& sdp)
{
    logVerbose("remote answer SDP received from server");
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            pcManager->setPublisherRemoteAnswer(std::move(desc));
        }
    }
    else {
        logError("failed to parse remote answer for publisher: " + error.description);
    }
}

void RTCEngine::onPong(uint64_t, int64_t, int64_t)
{
    logVerbose("ping/pong received pong from server");
    // Clear timeout timer
    _pingTimeoutTimer.stop();
}

void RTCEngine::onTrickle(uint64_t, const TrickleRequest& request)
{
    logVerbose("trickle ICE for " + std::string(toString(request._target)) +
               " received from server");
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        webrtc::SdpParseError error;
        if (auto candidate = RoomUtils::map(request, &error)) {
            pcManager->addRemoteIceCandidate(request._target, std::move(candidate));
        }
        else {
            logError("failed to parse ICE candidate SDP for " +
                     std::string(toString(request._target)) + ": " + error.description);
        }
    }
}

void RTCEngine::onTransportStateChanged(uint64_t, TransportState state)
{
    if (TransportState::Disconnected == state) {
        cleanup(false);
    }
}

void RTCEngine::onTransportError(uint64_t, std::string error)
{
    logError(error);
    cleanup(true);
}

void RTCEngine::onTimeout(MediaTimer* timer)
{
    if (&_pingIntervalTimer == timer) {
        const auto ljr = std::atomic_load(&_latestJoinResponse);
        if (ljr && sendPing()) {
            if (ljr->_pingTimeout > 0) {
                _pingTimeoutTimer.start(ljr->_pingTimeout * 1000);
            }
        }
    }
    else if (&_pingTimeoutTimer == timer) {
        logError("ping/pong timed out");
        cleanup(true);
        //await self.cleanUp(withError: LiveKitError(.serverPingTimedOut))
    }
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
