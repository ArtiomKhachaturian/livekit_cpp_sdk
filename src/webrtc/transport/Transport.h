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
#pragma once // Transport.h
#include "CreateSdpListener.h"
#include "Loggable.h"
#include "SetSdpListener.h"
#include "rtc/SignalTarget.h"
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

namespace webrtc {
class PeerConnectionObserver;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;
class CreateSdpObserver;
class SetLocalSdpObserver;
class SetRemoteSdpObserver;
class TransportListener;

class Transport : private Bricks::LoggableS<CreateSdpListener,
                                            SetSdpListener,
                                            webrtc::PeerConnectionObserver>
{
public:
    Transport(SignalTarget target, TransportListener* listener,
              const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
              const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
              const std::shared_ptr<Bricks::Logger>& logger = {});
    ~Transport() override;
    SignalTarget target() const noexcept { return _target; }
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    void createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    webrtc::PeerConnectionInterface::IceConnectionState iceConnectionState() const noexcept;
    webrtc::PeerConnectionInterface::SignalingState signalingState() const noexcept;
    webrtc::PeerConnectionInterface::IceGatheringState iceGatheringState() const noexcept;
    bool iceConnected() const noexcept;
    bool closed() const noexcept;
    std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> transceivers() const;
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers() const;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders() const;
    const webrtc::SessionDescriptionInterface* localDescription() const;
    const webrtc::SessionDescriptionInterface* remoteDescription() const;
    // getStats()
    bool valid() const { return nullptr != _pc; }
    explicit operator bool() const { return valid(); }
    void close();
    bool removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender);
    void addRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    rtc::scoped_refptr<webrtc::DataChannelInterface>
        createDataChannel(const std::string& label,
                          const webrtc::DataChannelInit& init = {},
                          webrtc::DataChannelObserver* observer = nullptr);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface>
        addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface>
        addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                       const webrtc::RtpTransceiverInit& init);
    rtc::scoped_refptr<webrtc::RtpSenderInterface>
        addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                 const std::vector<std::string>& streamIds = {},
                 const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings = {});
private:
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface>
        createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
    void logWebRTCError(const webrtc::RTCError& error, std::string_view prefix = {}) const;
    template<typename TState>
    bool changeAndLogState(TState newState, std::atomic<TState>& holder) const;
    // impl. of CreateSdpObserver
    void onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onFailure(webrtc::SdpType type, webrtc::RTCError error) final;
    // impl. of SetSdpListener
    void onCompleted(bool local) final;
    void onFailure(bool local, webrtc::RTCError error) final;
    // impl. of webrtc::PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) final;
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) final;
    void OnRenegotiationNeeded() final;
    void OnNegotiationNeededEvent(uint32_t eventId) final;
    void OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) final;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) final;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) final;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) final;
    void OnIceCandidateError(const std::string& address, int port, const std::string& url,
                             int errorCode, const std::string& errorText) final;
    void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) final;
    void OnIceConnectionReceivingChange(bool receiving) final;
    void OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) final;
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) final;
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    void OnInterestingUsage(int usagePattern) final;
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final { return _logCategory; }
private:
    const std::string _logCategory;
    const SignalTarget _target;
    TransportListener* const _listener;
    const webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
    webrtc::scoped_refptr<CreateSdpObserver> _offerCreationObserver;
    webrtc::scoped_refptr<CreateSdpObserver> _answerCreationObserver;
    webrtc::scoped_refptr<SetLocalSdpObserver> _setLocalSdpObserver;
    webrtc::scoped_refptr<SetRemoteSdpObserver> _setRemoteSdpObserver;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _pcState;
    std::atomic<webrtc::PeerConnectionInterface::IceConnectionState> _iceConnState;
    std::atomic<webrtc::PeerConnectionInterface::SignalingState> _signalingState;
    std::atomic<webrtc::PeerConnectionInterface::IceGatheringState> _iceGatheringState;
};

} // namespace LiveKitCpp
