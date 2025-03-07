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

class TransportListener : public webrtc::PeerConnectionObserver
{
public:
    virtual void onSdpCreated(Transport* transport, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) = 0;
    virtual void onSdpCreationFailure(Transport* transport, webrtc::SdpType type, webrtc::RTCError error) = 0;
    virtual void onSdpSet(Transport* transport, bool local, const webrtc::SessionDescriptionInterface* desc) = 0;
    virtual void onSdpSetFailure(Transport* transport, bool local, webrtc::RTCError error) = 0;
    virtual void onSetConfigurationError(Transport* transport, webrtc::RTCError error) = 0;
protected:
    virtual ~TransportListener() = default;
};

} // namespace LiveKitCpp
