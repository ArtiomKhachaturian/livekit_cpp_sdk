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
#pragma once // TransportManagerListener.h
#include "PingPongKitListener.h"
#include "DataChannel.h"
#include <api/peer_connection_interface.h>

namespace LiveKitCpp
{

enum class SignalTarget;

class TransportManagerListener : public PingPongKitListener
{
public:
    virtual void onSdpOperationFailed(SignalTarget /*target*/, webrtc::RTCError /*error*/) {}
    virtual void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState /*state*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*publisherState*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*subscriberState*/) {}
    virtual void onNegotiationNeeded() {}
    virtual void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> /*channel*/) {}
    virtual void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> /*channel*/) {}
    virtual void onIceCandidateGathered(SignalTarget /*target*/, std::string /*sdpMid*/,
                                        int /*sdpMlineIndex*/, cricket::Candidate /*candidate*/) {}
    virtual void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> /*sender*/) {}
    virtual void onLocalTrackAddFailure(std::string /*id*/,
                                        webrtc::MediaType /*type*/,
                                        std::vector<std::string> /*streamIds*/,
                                        webrtc::RTCError /*error*/) {}
    virtual void onLocalTrackRemoved(std::string /*id*/, webrtc::MediaType /*type*/) {}
    virtual void onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
                                    std::string /*trackId*/,
                                    std::string /*participantSid*/ = {}) {}
    virtual void onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) {}
    virtual void onPublisherOffer(std::string /*type*/, std::string /*sdp*/) {}
    virtual void onSubscriberAnswer(std::string /*type*/, std::string /*sdp*/) {}
protected:
    virtual ~TransportManagerListener() = default;
};

} // namespace LiveKitCpp
