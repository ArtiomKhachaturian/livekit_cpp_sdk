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
#include "LiveKitRoom.h"
#ifdef WEBRTC_AVAILABLE
#include "SignalClientWs.h"
#include "SignalServerListener.h"
#include "SignalTransportListener.h"
#include "Listeners.h"
#include "Loggable.h"
#include "ProtectedObjAliases.h"
#include "LiveKitRoomOptions.h"
#include "Transport.h"
#include "PeerConnectionFactory.h"
#include "websocket/WebsocketFactory.h"
#include "rtc/JoinResponse.h"
#endif
#include "websocket/Websocket.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE

using ImplBase = LoggableShared<SignalServerListener, SignalTransportListener, webrtc::PeerConnectionObserver>;

struct LiveKitRoom::Impl : public ImplBase
{
    Impl(std::unique_ptr<Websocket> socket,
         PeerConnectionFactory* pcf,
         const LiveKitRoomOptions& options);
    ~Impl();
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    const LiveKitRoomOptions _options;
    SignalClientWs _client;
    ProtectedUniquePtr<Transport> _publisher;
    ProtectedUniquePtr<Transport> _subscriber;
    webrtc::PeerConnectionInterface::RTCConfiguration makeConfig(const JoinResponse& response) const;
    // impl. of SignalServerListener
    void onJoin(uint64_t signalClientId, const JoinResponse& response) final;
    // impl. of webrtc::PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) final;
    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel) final;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) final;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) final;
private:
    static webrtc::PeerConnectionInterface::IceServer map(const ICEServer& server);
    static webrtc::PeerConnectionInterface::IceServers map(const std::vector<ICEServer>& servers);
    //std::vector<ICEServer>
};

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket> socket,
                         PeerConnectionFactory* pcf,
                         const LiveKitRoomOptions& options)
    : _impl(std::make_unique<Impl>(std::move(socket), pcf, options))
{
}

LiveKitRoom::~LiveKitRoom()
{
}

bool LiveKitRoom::connect(std::string host, std::string authToken)
{
    // dummy
    _impl->_client.setHost(std::move(host));
    _impl->_client.setAuthToken(std::move(authToken));
    return _impl->_client.connect();
}

void LiveKitRoom::disconnect()
{
    _impl->_client.disconnect();
}

LiveKitRoom::Impl::Impl(std::unique_ptr<Websocket> socket,
                        PeerConnectionFactory* pcf,
                        const LiveKitRoomOptions& options)
    : ImplBase(pcf->logger())
    , _pcf(pcf)
    , _client(std::move(socket), _pcf->logger().get())
    , _options(options)
{
    _client.setAdaptiveStream(options._adaptiveStream);
    _client.addServerListener(this);
    _client.addTransportListener(this);
}

LiveKitRoom::Impl::~Impl()
{
    _client.removeServerListener(this);
    _client.removeTransportListener(this);
}

webrtc::PeerConnectionInterface::RTCConfiguration LiveKitRoom::Impl::
    makeConfig(const JoinResponse& response) const
{
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.network_preference.emplace(::rtc::AdapterType::ADAPTER_TYPE_ETHERNET); // ethernet is preferred
    // set some defaults
    config.set_cpu_adaptation(true);
    //config.enable_dtls_srtp.emplace(true);
    // enable ICE renomination, like on Android ("a=ice-options:trickle renomination")
    config.enable_ice_renomination = true;
    // required for M86:
    // the issue may because 1 byte rtp header id exahustion, check if you have enabled this option:
    // https://source.chromium.org/chromium/chromium/src/+/master:third_party/webrtc/api/peer_connection_interface.h;l=625?q=RTCConfiguration%20webrtc&ss=chromium%2Fchromium%2Fsrc
    config.offer_extmap_allow_mixed = true;
    if (const auto& clientConf = response._clientConfiguration) {
        if (ClientConfigSetting::Enabled == clientConf->_forceRelay) {
            config.type = webrtc::PeerConnectionInterface::kRelay;
        }
        /*else {
            config.type = connectOptions.iceTransportPolicy.toRTCType()
        }*/
    }
    // Set iceServers provided by the server
    config.servers = map(response._iceServers);
    /*if !connectOptions.iceServers.isEmpty {
       // Override with user provided iceServers
       rtcConfiguration.iceServers = connectOptions.iceServers.map { $0.toRTCType() }
    }*/
    
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

void LiveKitRoom::Impl::onJoin(uint64_t signalClientId, const JoinResponse& response)
{
    SignalServerListener::onJoin(signalClientId, response);
    LOCK_WRITE_PROTECTED_OBJ(_publisher);
    LOCK_WRITE_PROTECTED_OBJ(_subscriber);
    // protocol v3
    // log("subscriberPrimary: \(joinResponse.subscriberPrimary)")
    const auto config = makeConfig(response);
    _publisher = Transport::create(response._subscriberPrimary, SignalTarget::Publisher,
                                   this, _pcf, config);
    _subscriber = Transport::create(!response._subscriberPrimary, SignalTarget::Subscriber,
                                   this, _pcf, config);
}

void LiveKitRoom::Impl::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    
}

void LiveKitRoom::Impl::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel)
{
    
}

void LiveKitRoom::Impl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    
}

void LiveKitRoom::Impl::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    
}

webrtc::PeerConnectionInterface::IceServer LiveKitRoom::Impl::map(const ICEServer& server)
{
    webrtc::PeerConnectionInterface::IceServer webrtcIceSrv;
    webrtcIceSrv.username = server._username;
    webrtcIceSrv.password = server._credential;
    webrtcIceSrv.urls = server._urls;
    return webrtcIceSrv;
}

webrtc::PeerConnectionInterface::IceServers LiveKitRoom::Impl::map(const std::vector<ICEServer>& servers)
{
    webrtc::PeerConnectionInterface::IceServers webrtcIceSrvs;
    webrtcIceSrvs.reserve(servers.size());
    for (const auto& server : servers) {
        webrtcIceSrvs.push_back(map(server));
    }
    return webrtcIceSrvs;
}

#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket>, PeerConnectionFactory*,
                         const LiveKitRoomOptions&) {}

LiveKitRoom::~LiveKitRoom() {}

#endif

} // namespace LiveKitCpp
