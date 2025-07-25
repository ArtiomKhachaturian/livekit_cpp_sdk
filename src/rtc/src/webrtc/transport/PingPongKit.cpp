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
#include "PingPongKit.h"
#include "PingPongKitImpl.h"

namespace LiveKitCpp
{

PingPongKit::PingPongKit(uint32_t pingInterval, uint32_t pingTimeout,
                         const webrtc::scoped_refptr<PeerConnectionFactory>& pcf)
    : RtcObject<PingPongKitImpl>(pingInterval, pingTimeout, pcf)
{
}

PingPongKit::~PingPongKit()
{
    if (auto impl = dispose()) {
        impl->stop();
    }
}

void PingPongKit::start(PingPongKitListener* listener)
{
    if (const auto impl = loadImpl()) {
        impl->start(listener);
    }
}

void PingPongKit::stop()
{
    if (const auto impl = loadImpl()) {
        impl->stop();
    }
}

void PingPongKit::notifyThatPongReceived()
{
    if (const auto impl = loadImpl()) {
        impl->notifyThatPongReceived();
    }
}

} // namespace LiveKitCpp
