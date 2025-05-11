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
#pragma once // SignalTransportListenerAsync.h
#include "AsyncListener.h"
#include "livekit/signaling/SignalTransportListener.h"

namespace LiveKitCpp
{

class PeerConnectionFactory;

class SignalTransportListenerAsync : public SignalTransportListener
{
public:
    SignalTransportListenerAsync(std::weak_ptr<webrtc::TaskQueueBase> queue);
    SignalTransportListenerAsync(const PeerConnectionFactory* pcf);
    void set(SignalTransportListener* listener);
    // impl. of SignalTransportListener
    void onTransportStateChanged(TransportState state) final;
    void onTransportError(std::string error) final;
private:
    AsyncListener<SignalTransportListener*> _impl;
};

} // namespace LiveKitCpp
