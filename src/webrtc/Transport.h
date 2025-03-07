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
#include "SafeObj.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/SignalTarget.h"
#include "rtc/ICEServer.h"
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_set>

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

using BaseTransport = Bricks::LoggableS<CreateSdpListener,
                      SetSdpListener,
                      webrtc::PeerConnectionObserver>;

class Transport : private BaseTransport
{
    using PendingCandidates = std::unordered_set<const webrtc::IceCandidateInterface*>;
public:
    Transport(bool primary, SignalTarget target, TransportListener* listener,
              const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
              const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
    ~Transport() override;
    bool primary() const noexcept { return _primary; }
    SignalTarget target() const noexcept { return _target; }
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    void createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setRestartingIce(bool restartingIce);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const;
    webrtc::PeerConnectionInterface::IceConnectionState iceConnectionState() const;
    webrtc::PeerConnectionInterface::SignalingState signalingState() const;
    std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> transceivers() const;
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers() const;
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders() const;
    const webrtc::SessionDescriptionInterface* localDescription() const;
    const webrtc::SessionDescriptionInterface* remoteDescription() const;
    // getStats()
    bool valid() const { return nullptr != _pc; }
    explicit operator bool() const { return valid(); }
    bool iceConnected() const;
    void close();
    bool removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender);
    bool addIceCandidate(const webrtc::IceCandidateInterface* candidate);
    rtc::scoped_refptr<webrtc::DataChannelInterface> createDataChannel(const std::string& label,
                                                                       const webrtc::DataChannelInit* init);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                                                                       const webrtc::RtpTransceiverInit& init);
    rtc::scoped_refptr<webrtc::RtpSenderInterface> addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                                                            const std::vector<std::string>& streamIds,
                                                            const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings = {});
private:
    void logWebRTCError(const webrtc::RTCError& error) const;
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
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) final;
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
    std::string_view logCategory() const final;
private:
    const bool _primary;
    const SignalTarget _target;
    TransportListener* const _listener;
    webrtc::scoped_refptr<CreateSdpObserver> _offerCreationObserver;
    webrtc::scoped_refptr<CreateSdpObserver> _answerCreationObserver;
    webrtc::scoped_refptr<SetLocalSdpObserver> _setLocalSdpObserver;
    webrtc::scoped_refptr<SetRemoteSdpObserver> _setRemoteSdpObserver;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
    std::atomic_bool _restartingIce = false;
    std::atomic_bool _renegotiate = false;
    Bricks::SafeObj<PendingCandidates> _pendingCandidates;
};

} // namespace LiveKitCpp
