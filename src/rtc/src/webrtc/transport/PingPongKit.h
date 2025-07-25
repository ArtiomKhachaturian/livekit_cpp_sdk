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
#pragma once // PingPongKit.h
#include "MediaTimer.h"
#include "MediaTimerCallback.h"
#include "RtcObject.h"


namespace LiveKitCpp
{

class PingPongKitListener;
class PingPongKitImpl;

class PingPongKit : private RtcObject<PingPongKitImpl>
{
public:
    // interval & timeout in seconds
    PingPongKit(uint32_t pingInterval, uint32_t pingTimeout,
                const webrtc::scoped_refptr<PeerConnectionFactory>& pcf);
    ~PingPongKit();
    void start(PingPongKitListener* listener);
    void stop();
    void notifyThatPongReceived();
};

} // namespace LiveKitCpp
