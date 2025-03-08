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
#include "rtc/ReconnectResponse.h"
#include <thread>

namespace LiveKitCpp
{

RTCEngine::RTCEngine(const SignalOptions& signalOptions,
                     PeerConnectionFactory* pcf,
                     std::unique_ptr<Websocket::EndPoint> socket,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportManagerListener,
                        SignalTransportListener,
                        SignalServerListener>(logger)
    , _signalOptions(signalOptions)
    , _pcf(pcf)
    , _localAudioTrack(createLocalAudioTrack())
    , _client(std::move(socket), logger.get())
{
    _client.setAdaptiveStream(_signalOptions._adaptiveStream);
    _client.setAutoSubscribe(_signalOptions._autoSubscribe);
    _client.setProtocolVersion(_signalOptions._protocolVersion);
    _client.setServerListener(this);
}

RTCEngine::~RTCEngine()
{
    _client.setServerListener(nullptr);
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
            if (_joinAttempts < _signalOptions._reconnectAttempts) {
                if (canLogWarning()) {
                    logWarning("Couldn't connect to server, attempt " +
                               std::to_string(_joinAttempts) + " of " +
                               std::to_string(_signalOptions._reconnectAttempts));
                }
                std::this_thread::sleep_for(_signalOptions._reconnectAttemptDelay);
                ok = connect(_client.host(), _client.authToken());
            }
        }
    }
    return ok;
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
        config.type = RoomUtils::map(_signalOptions._iceTransportPolicy);
    }
    if (!_signalOptions._iceServers.empty()) {
        // Override with user provided iceServers
        config.servers = RoomUtils::map(_signalOptions._iceServers);
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

void RTCEngine::onJoin(uint64_t, const JoinResponse& response)
{
    std::atomic_store(&_latestJoinResponse, std::make_shared<const JoinResponse>(response));
    TransportManagerListener* listener = this;
    LOCK_WRITE_SAFE_OBJ(_pcManager);
    _pcManager = std::make_unique<TransportManager>(response._subscriberPrimary,
                                                    listener, _pcf,
                                                    makeConfiguration(response),
                                                    logger());
    //_pcManager.constRef()->addTrack(_localAudioTrack);
    _pcManager.constRef()->createPublisherOffer();
}

void RTCEngine::onOffer(uint64_t, const SessionDescription& sdp)
{
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        LOCK_READ_SAFE_OBJ(_pcManager);
        if (const auto& pcManager = _pcManager.constRef()) {
            pcManager->setSubscriberRemoteOffer(std::move(desc));
        }
    }
    else if (canLogError()) {
        logError("failed to parse remote offer for subscriber: " + error.description);
    }
}

void RTCEngine::onAnswer(uint64_t, const SessionDescription& sdp)
{
    webrtc::SdpParseError error;
    if (auto desc = RoomUtils::map(sdp, &error)) {
        LOCK_READ_SAFE_OBJ(_pcManager);
        if (const auto& pcManager = _pcManager.constRef()) {
            pcManager->setPublisherRemoteAnswer(std::move(desc));
        }
    }
    else if (canLogError()) {
        logError("failed to parse remote answer for publisher: " + error.description);
    }
}

std::string_view RTCEngine::logCategory() const
{
    static const std::string_view category("RTCEngine");
    //LOCK_READ_SAFE_OBJ(_latestJoinResponse);
    /*if (const auto& resp = _latestJoinResponse->value()) {
        
    }*/
    return category;
}

} // namespace LiveKitCpp
