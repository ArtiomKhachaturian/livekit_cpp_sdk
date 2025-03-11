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
#include "DataChannel.h"
#include "rtc/SignalTarget.h"
#include "rtc/ReconnectResponse.h"
#include "rtc/Ping.h"
#include "rtc/TrickleRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/MuteTrackRequest.h"
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
                        DataChannelListener,
                        LocalTrackManager>(logger)
    , _options(signalOptions)
    , _pcf(pcf)
    , _client(std::move(socket), logger.get())
    , _microphone(this, true)
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
                std::this_thread::sleep_for(_options._reconnectAttemptDelay);
                ok = connect(_client.host(), _client.authToken());
            }
        }
    }
    return ok;
}

void RTCEngine::setMicrophoneEnabled(bool enable)
{
    _microphone.setEnabled(enable);
}

void RTCEngine::cleanup(bool /*error*/)
{
    std::atomic_store(&_pcManager, std::shared_ptr<TransportManager>());
    _client.disconnect();
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

void RTCEngine::onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        if (DataChannel::lossyDCLabel() == channel->label()) {
            channel->setListener(this);
            _lossyDC(std::move(channel));
        }
        else if (DataChannel::reliableDCLabel() == channel->label()) {
            channel->setListener(this);
            _reliableDC(std::move(channel));
        }
    }
}

void RTCEngine::onPublisherOffer(const webrtc::SessionDescriptionInterface* desc)
{
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
    if (const auto sdp = RoomUtils::map(desc)) {
        if (!_client.sendAnswer(sdp.value())) {
            logError("failed to send subscriber answer to the server");
        }
    }
    else {
        logError("failed to serialize subscriber answer into a string");
    }
}

void RTCEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            const LocalTrack* accepted = nullptr;
            SetSenderResult result = SetSenderResult::Rejected;
            switch (sender->media_type()) {
                case cricket::MEDIA_TYPE_AUDIO:
                    result = _microphone.setRequested(sender);
                    if (SetSenderResult::Accepted == result) {
                        accepted = &_microphone;
                    }
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
                     break;
                default:
                    break;
            }
            if (accepted) {
                AddTrackRequest request;
                if (accepted->fillRequest(request)) {
                    if (_client.sendAddTrack(request)) {
                        pcManager->negotiate(true);
                    }
                }
                else {
                    // TODO: log error
                }
            }
            else if (SetSenderResult::Rejected == result) {
                pcManager->removeTrack(std::move(sender));
            }
        }
    }
}

void RTCEngine::onLocalTrackAddFailure(const std::string& id,
                                       cricket::MediaType type,
                                       const std::vector<std::string>&,
                                       webrtc::RTCError)
{
    switch (type) {
        case cricket::MEDIA_TYPE_AUDIO:
            _microphone.notifyAboutRequestFailure(id);
            break;
        /*case cricket::MEDIA_TYPE_VIDEO:
            break;*/
        default:
            break;
    }
}

void RTCEngine::onIceCandidateGathered(SignalTarget target,
                                       const webrtc::IceCandidateInterface* candidate)
{
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

void RTCEngine::onJoin(uint64_t, const JoinResponse& response)
{
    TransportManagerListener* listener = this;
    auto pcManager = std::make_shared<TransportManager>(response, listener, _pcf,
                                                        makeConfiguration(response),
                                                        logger());
    std::atomic_store(&_pcManager, pcManager);
    pcManager->negotiate(false);
    pcManager->startPing();
}

void RTCEngine::onOffer(uint64_t, const SessionDescription& sdp)
{
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

void RTCEngine::onAnswer(uint64_t, const SessionDescription& sdp)
{
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

void RTCEngine::onPong(uint64_t, const Pong&)
{
    if (const auto pcManager = std::atomic_load(&_pcManager)) {
        // Clear timeout timer
        pcManager->notifyThatPongReceived();
    }
}

void RTCEngine::onTrickle(uint64_t, const TrickleRequest& request)
{
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

void RTCEngine::onTrackPublished(uint64_t, const TrackPublishedResponse& published)
{
    const LocalTrack* track = nullptr;
    switch (published._track._type) {
        case TrackType::Audio:
            if (published._cid == _microphone.cid()) {
                _microphone.setSid(published._track._sid);
                track = &_microphone;
            }
            break;
        case TrackType::Video:
            break;
        default:
            break;
    }
    // reconcile track mute status.
    // if server's track mute status doesn't match actual, we'll have to update
    // the server's copy
    if (track && track->muted() != published._track._muted) {
        notifyAboutEnabledChanges(*track);
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

void RTCEngine::onStateChange(DataChannel* channel)
{
    if (channel && canLogInfo()) {
        logInfo("the data channel '" + channel->label() + "' state have changed");
    }
}

void RTCEngine::onMessage(DataChannel* channel,
                                 const webrtc::DataBuffer& /*buffer*/)
{
    if (channel && canLogInfo()) {
        logInfo("a message buffer was successfully received for '" +
                channel->label() + " data channel");
    }
}

void RTCEngine::onBufferedAmountChange(DataChannel* /*channelType*/,
                                       uint64_t /*sentDataSize*/)
{
}

bool RTCEngine::add(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    if (track) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            return pcManager->addTrack(std::move(track));
        }
    }
    return false;
}

bool RTCEngine::remove(webrtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender) {
        if (const auto pcManager = std::atomic_load(&_pcManager)) {
            return pcManager->removeTrack(std::move(sender));
        }
    }
    return false;
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> RTCEngine::createAudio(const std::string& label,
                                                                          const cricket::AudioOptions& options)
{
    if (_pcf) {
        if (const auto source = _pcf->CreateAudioSource(options)) {
            return _pcf->CreateAudioTrack(label, source.get());
        }
    }
    return {};
}

void RTCEngine::notifyAboutEnabledChanges(const LocalTrack& track)
{
    _client.sendMuteTrack({._sid = track.sid(), ._muted = track.muted()});
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
