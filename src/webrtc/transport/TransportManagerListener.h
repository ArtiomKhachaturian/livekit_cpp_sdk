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
#include <api/peer_connection_interface.h>

namespace LiveKitCpp
{

enum class SignalTarget;
class DataChannel;

class TransportManagerListener : public PingPongKitListener
{
public:
    virtual void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState /*state*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*publisherState*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*subscriberState*/) {}
    virtual void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> /*channel*/) {}
    virtual void onIceCandidateGathered(SignalTarget /*target*/,
                                        const webrtc::IceCandidateInterface* /*candidate*/) {}
    virtual void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> /*sender*/) {}
    virtual void onLocalTrackRemoved(const std::string& /*id*/, cricket::MediaType /*type*/) {}
    virtual void onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}
    virtual void onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) {}
    virtual void onPublisherOffer(const webrtc::SessionDescriptionInterface* /*desc*/) {}
    virtual void onSubscriberAnswer(const webrtc::SessionDescriptionInterface* /*desc*/) {}
protected:
    virtual ~TransportManagerListener() = default;
};

} // namespace LiveKitCpp
