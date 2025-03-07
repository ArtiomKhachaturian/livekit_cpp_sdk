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
#include <api/peer_connection_interface.h>

namespace LiveKitCpp
{

enum class SignalTarget;
class TransportManager;

class TransportManagerListener
{
public:
    virtual void onStateChange(TransportManager& /*manager*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*state*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*publisherState*/,
                               webrtc::PeerConnectionInterface::PeerConnectionState /*subscriberState*/) {}
    virtual void onDataChannel(TransportManager& /*manager*/,
                               rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) {}
    virtual void onIceCandidate(TransportManager& /*manager*/,
                                SignalTarget /*target*/,
                                const webrtc::IceCandidateInterface* /*candidate*/) {}
    virtual void onRemoteTrack(TransportManager& /*manager*/,
                               rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}
    virtual void onPublisherOffer(TransportManager& /*manager*/,
                                  const webrtc::SessionDescriptionInterface* /*desc*/) {}
    virtual void onSubscriberAnswer(TransportManager& /*manager*/,
                                    const webrtc::SessionDescriptionInterface* /*desc*/) {}
protected:
    virtual ~TransportManagerListener() = default;
};

} // namespace LiveKitCpp
