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
#include "TransportListener.h"
#include "Listeners.h"
#include "Loggable.h"
#include "SafeObjAliases.h"
#include "RoomOptions.h"
#include "RoomUtils.h"
#include "Transport.h"
#include "TransportListener.h"
#include "LiveKitRoomState.h"
#include "PeerConnectionFactory.h"
#include "WebsocketFactory.h"
#include "rtc/JoinResponse.h"
#include "rtc/SessionDescription.h"
#endif
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE

using ImplBase = Bricks::LoggableS<SignalServerListener,
                                   SignalTransportListener,
                                   TransportListener>;

struct LiveKitRoom::Impl : public ImplBase
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const ConnectOptions& connectOptions,
         const RoomOptions& roomOptions);
    ~Impl();
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    SignalClientWs _client;
    Bricks::SafeObj<LiveKitRoomState> _state;
    // impl. of SignalServerListener
    void onJoin(uint64_t signalClientId, const JoinResponse& response) final;
    void onReconnect(uint64_t signalClientId, const ReconnectResponse& response) final;
    void onOffer(uint64_t signalClientId, const SessionDescription& sdp) final;
    // impl. of TransportListener
    void onSdpCreated(Transport* transport, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onSdpCreationFailure(Transport* transport, webrtc::SdpType type, webrtc::RTCError error) final;
    void onSdpSet(Transport* transport, bool local, const webrtc::SessionDescriptionInterface* desc) final;
    void onSdpSetFailure(Transport* transport, bool local, webrtc::RTCError error) final;
    void onSetConfigurationError(Transport* transport, webrtc::RTCError error) final;
    // impl. of webrtc::PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) final;
    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> dataChannel) final;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) final;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) final;
};

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint> socket,
                         PeerConnectionFactory* pcf,
                         const ConnectOptions& connectOptions,
                         const RoomOptions& roomOptions)
    : _impl(std::make_unique<Impl>(std::move(socket), pcf, connectOptions, roomOptions))
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

LiveKitRoom::Impl::Impl(std::unique_ptr<Websocket::EndPoint> socket,
                        PeerConnectionFactory* pcf,
                        const ConnectOptions& connectOptions,
                        const RoomOptions& roomOptions)
    : ImplBase(pcf->logger())
    , _pcf(pcf)
    , _state(connectOptions, roomOptions)
    , _client(std::move(socket), _pcf->logger().get())
{
    _client.setAdaptiveStream(roomOptions._adaptiveStream);
    _client.setAutoSubscribe(connectOptions._autoSubscribe);
    _client.setProtocolVersion(connectOptions._protocolVersion);
    _client.addServerListener(this);
    _client.addTransportListener(this);
}

LiveKitRoom::Impl::~Impl()
{
    _client.removeServerListener(this);
    _client.removeTransportListener(this);
}

void LiveKitRoom::Impl::onJoin(uint64_t signalClientId, const JoinResponse& response)
{
    // https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Core/Room%2BEngine.swift#L118
    SignalServerListener::onJoin(signalClientId, response);
    LOCK_WRITE_SAFE_OBJ(_state);
    // protocol v3
    // log("subscriberPrimary: \(joinResponse.subscriberPrimary)")
    const auto config = _state->makeConfiguration(response);
    _state->_publisher = Transport::create(response._subscriberPrimary,
                                           SignalTarget::Publisher,
                                           this, _pcf, config);
    if (!_state->_publisher) {
        return;
    }
    _state->_subscriber = Transport::create(!response._subscriberPrimary,
                                            SignalTarget::Subscriber,
                                            this, _pcf, config);
    if (!_state->_subscriber) {
        _state->_publisher.reset();
        return;
    }
    if (const auto& room = response._room) {
        _state->_sid = room->_sid;
        _state->_name = room->_name;
        _state->_creationTime = std::chrono::system_clock::from_time_t(static_cast<time_t>(room->_creationTime));
        _state->_maxParticipants = room->_maxParticipants;
        _state->_metadata = room->_metadata;
        _state->_isRecording = room->_activeRecording;
        _state->_numParticipants = room->_numParticipants;
        _state->_numPublishers = room->_numPublishers;
    }
    else {
        _state->_sid = {};
        _state->_name = {};
        _state->_creationTime = {};
        _state->_maxParticipants = {};
        _state->_metadata = {};
        _state->_isRecording = {};
        _state->_numParticipants = {};
        _state->_numPublishers = {};
    }
    /*if e2eeManager != nil, !joinResponse.sifTrailer.isEmpty {
                    e2eeManager?.keyProvider.setSifTrailer(trailer: joinResponse.sifTrailer)
                }*/
    // localParticipant.set(info: joinResponse.participant, connectionState: $0.connectionState)
    /*if !joinResponse.otherParticipants.isEmpty {
                        for otherParticipant in joinResponse.otherParticipants {
                            $0.updateRemoteParticipant(info: otherParticipant, room: self)
                        }
                    }*/
    _state->_publisher->createOffer();
    // data over pub channel for backwards compatibility

    /*let reliableDataChannel = await publisher.dataChannel(for: LKRTCDataChannel.labels.reliable,
                                                          configuration: RTC.createDataChannelConfiguration())

    let lossyDataChannel = await publisher.dataChannel(for: LKRTCDataChannel.labels.lossy,
                                                       configuration: RTC.createDataChannelConfiguration(maxRetransmits: 0))

    publisherDataChannel.set(reliable: reliableDataChannel)
    publisherDataChannel.set(lossy: lossyDataChannel)

    log("dataChannel.\(String(describing: reliableDataChannel?.label)) : \(String(describing: reliableDataChannel?.channelId))")
    log("dataChannel.\(String(describing: lossyDataChannel?.label)) : \(String(describing: lossyDataChannel?.channelId))")*/
    _state->_isSubscriberPrimary = response._subscriberPrimary;
    
    /*if !isSubscriberPrimary {
        // lazy negotiation for protocol v3+
        try await publisherShouldNegotiate()
    }*/
}

void LiveKitRoom::Impl::onReconnect(uint64_t signalClientId, const ReconnectResponse& response)
{
    SignalServerListener::onReconnect(signalClientId, response);
    LOCK_READ_SAFE_OBJ(_state);
    if (_state->_publisher && _state->_subscriber) {
        const auto config = _state->makeConfiguration(response);
        _state->_publisher->setConfiguration(config);
        _state->_subscriber->setConfiguration(config);
    }
}

void LiveKitRoom::Impl::onOffer(uint64_t signalClientId, const SessionDescription& sdp)
{
    SignalServerListener::onOffer(signalClientId, sdp);
    LOCK_READ_SAFE_OBJ(_state);
    if (const auto& subscruber = _state->_subscriber) {
        webrtc::SdpParseError error;
        if (auto desc = RoomUtils::map(sdp, &error)) {
            subscruber->setRemoteDescription(std::move(desc));
            subscruber->createAnswer();
        }
        else {
            // TODO: log error
        }
    }
}

void LiveKitRoom::Impl::onSdpCreated(Transport* transport,
                                     std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (transport) {
        transport->setLocalDescription(std::move(desc));
    }
}

void LiveKitRoom::Impl::onSdpCreationFailure(Transport* transport, webrtc::SdpType type,
                                             webrtc::RTCError error)
{
}

void LiveKitRoom::Impl::onSdpSet(Transport* transport, bool local,
                                 const webrtc::SessionDescriptionInterface* desc)
{
    if (local && transport) {
        switch (transport->target()) {
            case SignalTarget::Publisher:
                if (const auto sdp = RoomUtils::map(desc)) {
                    _client.sendOffer(sdp.value());
                }
                break;
            case SignalTarget::Subscriber:
                if (const auto sdp = RoomUtils::map(desc)) {
                    _client.sendAnswer(sdp.value());
                }
                break;
        }
    }
}

void LiveKitRoom::Impl::onSdpSetFailure(Transport* transport, bool local,
                                        webrtc::RTCError error)
{
    
}

void LiveKitRoom::Impl::onSetConfigurationError(Transport* transport, webrtc::RTCError error)
{
    
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

#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint>,
                         PeerConnectionFactory*,
                         const ConnectOptions&,
                         const RoomOptions&) {}

LiveKitRoom::~LiveKitRoom() {}

bool LiveKitRoom::connect(std::string, std::string) { return false; }

void LiveKitRoom::disconnect() {}

#endif

} // namespace LiveKitCpp
