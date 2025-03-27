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
#include "Listener.h"
#include "PingPongKit.h"
#include "PingPongKitListener.h"
#include "SafeScopedRefPtr.h"
#include "TransportListener.h"
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class TransportManagerListener;
class PeerConnectionFactory;
class DataChannelObserver;
enum class SignalTarget;

class TransportManager : private Bricks::LoggableS<TransportListener, PingPongKitListener>
{
public:
    TransportManager(bool subscriberPrimary, bool fastPublish,
                     int32_t pingTimeout, int32_t pingInterval,
                     uint64_t negotiationDelay, // ms
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::string& identity,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~TransportManager() final;
    bool valid() const noexcept;
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    bool closed() const noexcept;
    void negotiate(bool force = false);
    void startPing() { _pingPongKit.start(); }
    void stopPing() { _pingPongKit.stop(); }
    void notifyThatPongReceived() { _pingPongKit.notifyThatPongReceived(); }
    bool setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    bool removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    void addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    void setAudioPlayout(bool playout);
    void setAudioRecording(bool recording);
    void close();
    void setListener(TransportManagerListener* listener);
private:
    void createPublisherOffer();
    bool canNegotiate() const noexcept;
    bool localDataChannelsAreCreated() const noexcept { return _embeddedDCMaxCount == _embeddedDCCount; }
    Transport& primaryTransport() noexcept;
    const Transport& primaryTransport() const noexcept;
    bool isPrimary(SignalTarget target) const noexcept;
    void updateState();
    // impl. of TransportListener
    void onSdpCreated(SignalTarget target, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onSdpCreationFailure(SignalTarget target, webrtc::SdpType type, webrtc::RTCError error) final;
    void onSdpSet(SignalTarget target, bool local, const webrtc::SessionDescriptionInterface* desc) final;
    void onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error) final;
    void onTransceiverAdded(SignalTarget target,
                            rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void onTransceiverAddFailure(SignalTarget target,
                                 const std::string& id,
                                 cricket::MediaType type,
                                 const webrtc::RtpTransceiverInit& init,
                                 webrtc::RTCError error) final;
    void onLocalTrackRemoved(SignalTarget target, const std::string& id, cricket::MediaType type,
                             const std::vector<std::string>&) final;
    void onLocalDataChannelCreated(SignalTarget target,
                                   rtc::scoped_refptr<DataChannel> channel) final;
    void onConnectionChange(SignalTarget, webrtc::PeerConnectionInterface::PeerConnectionState) final;
    void onIceConnectionChange(SignalTarget, webrtc::PeerConnectionInterface::IceConnectionState) final;
    void onSignalingChange(SignalTarget, webrtc::PeerConnectionInterface::SignalingState) final;
    void onNegotiationNeededEvent(SignalTarget target, uint32_t eventId) final;
    void onRemoteDataChannelOpened(SignalTarget target,
                                   rtc::scoped_refptr<DataChannel> channel) final;
    void onIceCandidateGathered(SignalTarget target, const webrtc::IceCandidateInterface* candidate) final;
    void onRemoteTrackAdded(SignalTarget target, rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void onRemotedTrackRemoved(SignalTarget target,
                               rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    // impl. of PingPongKitListener
    bool onPingRequested() final;
    void onPongTimeout() final;
    // override of Bricks::LoggableS<>
    std::string_view logCategory() const final { return _logCategory; }
private:
    static constexpr uint8_t _embeddedDCMaxCount = 2U;
    const uint64_t _negotiationTimerId;
    const uint64_t _negotiationDelay;
    const bool _subscriberPrimary;
    const bool _fastPublish;
    const std::string _logCategory;
    const std::unique_ptr<MediaTimer> _negotiationTimer;
    Bricks::Listener<TransportManagerListener*> _listener;
    Transport _publisher;
    Transport _subscriber;
    PingPongKit _pingPongKit;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _state;
    std::atomic<uint8_t> _embeddedDCCount = 0U;
    std::atomic_bool _pendingNegotiation = false;
    std::atomic_bool _embeddedDCRequested = false;
};

} // namespace LiveKitCpp
