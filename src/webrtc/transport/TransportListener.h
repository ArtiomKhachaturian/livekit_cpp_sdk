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
enum class SignalTarget;

class TransportListener
{
public:
    virtual void onSdpCreated(SignalTarget /*target*/,
                              std::unique_ptr<webrtc::SessionDescriptionInterface> /*desc*/) {}
    virtual void onSdpCreationFailure(SignalTarget /*target*/,
                                      webrtc::SdpType /*type*/,
                                      webrtc::RTCError /*error*/) {}
    virtual void onSdpSet(SignalTarget /*target*/, bool /*local*/,
                          const webrtc::SessionDescriptionInterface* /*desc*/) {}
    virtual void onSdpSetFailure(SignalTarget /*target*/, bool /*local*/,
                                 webrtc::RTCError /*error*/) {}
    virtual void onConfigurationSet(SignalTarget /*target*/,
                                    const webrtc::PeerConnectionInterface::RTCConfiguration& /*config*/) {}
    virtual void onConfigurationSetFailure(SignalTarget /*target*/,
                                           webrtc::RTCError /*error*/) {}
    virtual void onIceCandidateAdded(SignalTarget /*target*/) {}
    virtual void onIceCandidateAddFailure(SignalTarget /*target*/,
                                          webrtc::RTCError /*error*/) {}
    virtual void onDataChannelCreated(SignalTarget /*target*/,
                                      rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) {}
    virtual void onDataChannelCreationFailure(SignalTarget /*target*/,
                                              const std::string& /*label*/,
                                              const webrtc::DataChannelInit& /*init*/,
                                              webrtc::RTCError /*error*/) {}
    virtual void onLocalTrackAdded(SignalTarget /*target*/,
                                   rtc::scoped_refptr<webrtc::RtpSenderInterface> /*sender*/) {}
    virtual void onLocalTrackAddFailure(SignalTarget /*target*/,
                                        const std::string& /*id*/,
                                        const std::string& /*kind*/,
                                        const std::vector<std::string>& /*streamIds*/,
                                        webrtc::RTCError /*error*/) {}
    virtual void onLocalTrackRemoved(SignalTarget /*target*/,
                                     const std::string& /*id*/,
                                     const std::string& /*kind*/,
                                     const std::vector<std::string>& /*streamIds*/) {}
    virtual void onLocalTrackRemoveFailure(SignalTarget /*target*/,
                                           const std::string& /*id*/,
                                           const std::string& /*kind*/,
                                           const std::vector<std::string>& /*streamIds*/,
                                           webrtc::RTCError /*error*/) {}
    virtual void onTransceiverAdded(SignalTarget /*target*/,
                                    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}
    virtual void onTransceiverAddFailure(SignalTarget /*target*/,
                                         const std::string& /*id*/,
                                         const std::string& /*kind*/,
                                         const webrtc::RtpTransceiverInit& /*init*/,
                                         webrtc::RTCError /*error*/) {}
    // extension of webrtc::PeerConnectionObserver interface
    virtual void onSignalingChange(SignalTarget /*target*/,
                                   webrtc::PeerConnectionInterface::SignalingState /*newState*/) {}
    virtual void onAddStream(SignalTarget /*target*/,
                             rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}
    virtual void onRemoveStream(SignalTarget /*target*/,
                                rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}
    virtual void onDataChannel(SignalTarget /*target*/,
                               rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) {}
    virtual void onNegotiationNeededEvent(SignalTarget /*target*/, uint32_t /*eventId*/) {}
    virtual void onIceConnectionChange(SignalTarget /*target*/,
                                       webrtc::PeerConnectionInterface::IceConnectionState /*newState*/) {}
    virtual void onConnectionChange(SignalTarget /*target*/,
                                    webrtc::PeerConnectionInterface::PeerConnectionState /*newState*/) {}
    virtual void onIceGatheringChange(SignalTarget /*target*/,
                                      webrtc::PeerConnectionInterface::IceGatheringState /*newState*/) {}
    virtual void onIceCandidate(SignalTarget /*target*/,
                                const webrtc::IceCandidateInterface* /*candidate*/) {}
    virtual void onIceCandidateError(SignalTarget /*target*/,
                                     const std::string& /*address*/, int /*port*/,
                                     const std::string& /*url*/,
                                     int /*errorCode*/, const std::string& /*errorText*/) {}
    virtual void onIceCandidatesRemoved(SignalTarget /*target*/,
                                        const std::vector<cricket::Candidate>& /*candidates*/) {}
    virtual void onIceConnectionReceivingChange(SignalTarget /*target*/,
                                                bool /*receiving*/) {}
    virtual void onIceSelectedCandidatePairChanged(SignalTarget /*target*/,
                                                   const cricket::CandidatePairChangeEvent& /*event*/) {}
    virtual void onAddTrack(SignalTarget /*target*/,
                            rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& /*streams*/) {}
    virtual void onTrack(SignalTarget /*target*/,
                         rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}
    virtual void onRemoveTrack(SignalTarget /*target*/,
                               rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) {}
    virtual void onInterestingUsage(SignalTarget /*target*/,
                                    int /*usagePattern*/) {}
protected:
    virtual ~TransportListener() = default;
};

} // namespace LiveKitCpp
