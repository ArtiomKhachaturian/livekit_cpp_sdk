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
#include "ProtectedObjAliases.h"
#include "RoomOptions.h"
#include "Transport.h"
#include "TransportListener.h"
#include "LiveKitRoomState.h"
#include "PeerConnectionFactory.h"
#include "WebsocketFactory.h"
#include "rtc/JoinResponse.h"
#endif
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE

using ImplBase = LoggableS<SignalServerListener,
                           SignalTransportListener,
                           TransportListener,
                           webrtc::PeerConnectionObserver>;

struct LiveKitRoom::Impl : public ImplBase
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const ConnectOptions& connectOptions,
         const RoomOptions& roomOptions);
    ~Impl();
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    ProtectedObj<LiveKitRoomState> _state;
    SignalClientWs _client;
    // impl. of SignalServerListener
    void onJoin(uint64_t signalClientId, const JoinResponse& response) final;
    void onReconnect(uint64_t signalClientId, const ReconnectResponse& response) final;
    // impl. of TransportListener
    void onSdpCreated(SignalTarget target, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onSdpCreationFailure(SignalTarget target, webrtc::SdpType type, webrtc::RTCError error) final;
    void onSdpSet(SignalTarget target, bool local) final;
    void onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error) final;
    void onSetConfigurationError(SignalTarget target, webrtc::RTCError error) final;
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
    LOCK_WRITE_PROTECTED_OBJ(_state);
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
    _state->_publisher->setListener(this);
    _state->_publisher->setListener(this);
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
    LOCK_READ_PROTECTED_OBJ(_state);
    if (_state->_publisher && _state->_subscriber) {
        const auto config = _state->makeConfiguration(response);
        _state->_publisher->setConfiguration(config);
        _state->_subscriber->setConfiguration(config);
    }
}

void LiveKitRoom::Impl::onSdpCreated(SignalTarget target,
                                     std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    LOCK_READ_PROTECTED_OBJ(_state);
    switch (target) {
        case SignalTarget::Publisher:
            if (const auto& publisher = _state->_publisher) {
                publisher->setLocalDescription(std::move(desc));
            }
            break;
        case SignalTarget::Subscriber:
            if (const auto& subscriber = _state->_subscriber) {
                subscriber->setLocalDescription(std::move(desc));
            }
            break;
    }
}

void LiveKitRoom::Impl::onSdpCreationFailure(SignalTarget target, webrtc::SdpType type,
                                             webrtc::RTCError error)
{
    
}

void LiveKitRoom::Impl::onSdpSet(SignalTarget target, bool local)
{
    
}

void LiveKitRoom::Impl::onSdpSetFailure(SignalTarget target, bool local,
                                        webrtc::RTCError error)
{
    
}

void LiveKitRoom::Impl::onSetConfigurationError(SignalTarget target, webrtc::RTCError error)
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
                         const RoomOptions&) {}

LiveKitRoom::~LiveKitRoom() {}

bool LiveKitRoom::connect(std::string, std::string) { return false; }

void LiveKitRoom::disconnect() {}

#endif

} // namespace LiveKitCpp
