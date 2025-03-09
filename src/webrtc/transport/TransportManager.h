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
#pragma once // TransportManager.h
#include "Loggable.h"
#include "Transport.h"
#include "TransportListener.h"
#include "DataChannelListener.h"
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class TransportManagerListener;
class PeerConnectionFactory;
class DataChannelObserver;
enum class SignalTarget;

class TransportManager : private Bricks::LoggableS<TransportListener, DataChannelListener>
{
public:
    TransportManager(bool subscriberPrimary,
                     TransportManagerListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~TransportManager() final;
    bool valid() const noexcept;
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    void negotiate(bool fastPublish);
    bool setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    /*void createPublisherOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createSubscriberAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    bool setSubscriberRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool setPublisherRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);*/
    rtc::scoped_refptr<webrtc::RtpSenderInterface> addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    void addRemoteIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    void close();
private:
    auto& primaryTransport() noexcept { return _subscriberPrimary ? _subscriber : _publisher; }
    const auto& primaryTransport() const noexcept { return _subscriberPrimary ? _subscriber : _publisher; }
    void updateState();
    // impl. of TransportListener
    void onSdpCreated(Transport& transport, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onSdpCreationFailure(Transport& transport, webrtc::SdpType type, webrtc::RTCError error) final;
    void onSdpSet(Transport& transport, bool local, const webrtc::SessionDescriptionInterface* desc) final;
    void onSdpSetFailure(Transport& transport, bool local, webrtc::RTCError error) final;
    void onSetConfigurationError(Transport& transport, webrtc::RTCError error) final;
    void onConnectionChange(Transport&, webrtc::PeerConnectionInterface::PeerConnectionState) final;
    void onIceConnectionChange(Transport&, webrtc::PeerConnectionInterface::IceConnectionState) final;
    void onSignalingChange(Transport&, webrtc::PeerConnectionInterface::SignalingState) final;
    void onDataChannel(Transport& transport, rtc::scoped_refptr<webrtc::DataChannelInterface> channel) final;
    void onIceCandidate(Transport& transport, const webrtc::IceCandidateInterface* candidate) final;
    void onTrack(Transport& transport, rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    // impl. of DataChannelListener
    void onStateChange(DataChannelType channelType) final;
    void onMessage(DataChannelType channelType, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannelType channelType, uint64_t sentDataSize) final;
    // override of Bricks::LoggableS<>
    std::string_view logCategory() const;
private:
    TransportManagerListener* const _listener;
    const bool _subscriberPrimary;
    Transport _publisher;
    Transport _subscriber;
    std::unique_ptr<DataChannelObserver> _lossyDCObserver;
    std::unique_ptr<DataChannelObserver> _reliableDCObserver;
    rtc::scoped_refptr<webrtc::DataChannelInterface> _lossyDC;
    rtc::scoped_refptr<webrtc::DataChannelInterface> _reliableDC;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _state;
};

} // namespace LiveKitCpp
