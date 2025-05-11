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
#include "SignalTransportListenerAsync.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

SignalTransportListenerAsync::SignalTransportListenerAsync(std::weak_ptr<webrtc::TaskQueueBase> queue)
    : _impl(std::move(queue))
{
}

SignalTransportListenerAsync::SignalTransportListenerAsync(const PeerConnectionFactory* pcf)
    : SignalTransportListenerAsync(pcf ? pcf->eventsQueue() : std::weak_ptr<webrtc::TaskQueueBase>{})
{
}

void SignalTransportListenerAsync::set(SignalTransportListener* listener)
{
    if (listener != this) {
        _impl.set(listener);
    }
}

void SignalTransportListenerAsync::onTransportStateChanged(TransportState state)
{
    _impl.invoke(&SignalTransportListener::onTransportStateChanged, state);
}

void SignalTransportListenerAsync::onTransportError(std::string error)
{
    _impl.invoke(&SignalTransportListener::onTransportError, std::move(error));
}

} // namespace LiveKitCpp
