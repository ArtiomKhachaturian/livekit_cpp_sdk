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
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class TransportManagerListener;
class PeerConnectionFactory;

class TransportManager : private Bricks::LoggableS<TransportListener>
{
public:
    TransportManager(bool subscriberPrimary,
                     TransportManagerListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
    ~TransportManager() final;
    bool valid() const noexcept;
    const Transport& publisher() const noexcept { return _publisher; }
    const Transport& subscriber() const noexcept { return _subscriber; }
    explicit operator bool() const noexcept { return valid(); }
    auto state() const noexcept { return _state.load(); }
    bool needsPublisher() const noexcept { return _publisherConnectionRequired; }
    bool needsSubscriber() const noexcept { return _subscriberConnectionRequired; }
    void requirePublisher(bool require = true);
    void requireSubscriber(bool require = true);
    bool addIceCandidate(const webrtc::IceCandidateInterface* candidate, SignalTarget target);
    void createAndSetPublisherOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createSubscriberAnswerFromOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setPublisherAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool updateConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config, bool iceRestart);
    void close();
    void triggerIceRestart();
    rtc::scoped_refptr<webrtc::DataChannelInterface>
        createPublisherDataChannel(const std::string& label,
                                   const webrtc::DataChannelInit& init = {},
                                   webrtc::DataChannelObserver* observer = nullptr);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> addPublisherTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface>
        addPublisherTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                                const webrtc::RtpTransceiverInit& init);
    rtc::scoped_refptr<webrtc::RtpSenderInterface>
        addPublisherTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                          const std::vector<std::string>& streamIds,
                          const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings = {});
private:
    void updateState();
    std::vector<const Transport*> requiredTransports() const;
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
private:
    TransportManagerListener* const _listener;
    Transport _publisher;
    Transport _subscriber;
    std::atomic_bool _publisherConnectionRequired = {};
    std::atomic_bool _subscriberConnectionRequired = {};
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _state;
};

} // namespace LiveKitCpp
