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
#pragma once // TransportListener.h
#include <api/jsep.h>
#include <api/peer_connection_interface.h>
#include <memory>

namespace LiveKitCpp
{

class Transport;

class TransportListener
{
public:
    virtual void onSdpCreated(Transport& /*transport*/,
                              std::unique_ptr<webrtc::SessionDescriptionInterface> /*desc*/) {}
    virtual void onSdpCreationFailure(Transport& /*transport*/,
                                      webrtc::SdpType /*type*/,
                                      webrtc::RTCError /*error*/) {}
    virtual void onSdpSet(Transport& /*transport*/, bool /*local*/,
                          const webrtc::SessionDescriptionInterface* /*desc*/) {}
    virtual void onSdpSetFailure(Transport& /*transport*/, bool /*local*/,
                                 webrtc::RTCError /*error*/) {}
    virtual void onSetConfigurationError(Transport& /*transport*/,
                                         webrtc::RTCError /*error*/) {}
    virtual void onRemoteIceCandidateAddFailed(Transport& /*transport*/,
                                               webrtc::RTCError /*error*/) {}
    // extension of webrtc::PeerConnectionObserver interface
    virtual void onSignalingChange(Transport& /*transport*/,
                                   webrtc::PeerConnectionInterface::SignalingState /*newState*/) {}
    virtual void onAddStream(Transport& /*transport*/,
                             rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}
    virtual void onRemoveStream(Transport& /*transport*/,
                                rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}
    virtual void onDataChannel(Transport& /*transport*/,
                               rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) {}
    virtual void onRenegotiationNeeded(Transport& /*transport*/) {}
    virtual void onNegotiationNeededEvent(Transport& /*transport*/, uint32_t /*eventId*/) {}
    virtual void onIceConnectionChange(Transport& /*transport*/,
                                       webrtc::PeerConnectionInterface::IceConnectionState /*newState*/) {}
    virtual void onConnectionChange(Transport& /*transport*/,
                                    webrtc::PeerConnectionInterface::PeerConnectionState /*newState*/) {}
    virtual void onIceGatheringChange(Transport& /*transport*/,
                                      webrtc::PeerConnectionInterface::IceGatheringState /*newState*/) {}
    virtual void onIceCandidate(Transport& /*transport*/,
                                const webrtc::IceCandidateInterface* /*candidate*/) {}
    virtual void onIceCandidateError(Transport& /*transport*/,
                                     const std::string& /*address*/, int /*port*/,
                                     const std::string& /*url*/,
                                     int /*errorCode*/, const std::string& /*errorText*/) {}
    virtual void onIceCandidatesRemoved(Transport& /*transport*/,
                                        const std::vector<cricket::Candidate>& /*candidates*/) {}
    virtual void onIceConnectionReceivingChange(Transport& /*transport*/,
                                                bool /*receiving*/) {}
    virtual void onIceSelectedCandidatePairChanged(Transport& /*transport*/,
                                                   const cricket::CandidatePairChangeEvent& /*event*/) {}
    virtual void onAddTrack(Transport& /*transport*/,
                            rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& /*streams*/) {}
    virtual void onTrack(Transport& /*transport*/,
                         rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}
    virtual void onRemoveTrack(Transport& /*transport*/,
                               rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) {}
    virtual void onInterestingUsage(Transport& /*transport*/,
                                    int /*usagePattern*/) {}
protected:
    virtual ~TransportListener() = default;
};

} // namespace LiveKitCpp
