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


namespace LiveKitCpp
{

class PingPongKitListener;

class PingPongKit : private MediaTimerCallback
{
public:
    PingPongKit(PingPongKitListener* listener,
                uint32_t pingInterval, uint32_t pingTimeout, // interval & timeout in seconds
                const webrtc::scoped_refptr<PeerConnectionFactory>& pcf);
    ~PingPongKit() final;
    void start();
    void stop();
    void notifyThatPongReceived();
private:
    // impl. of MediaTimerCallback
    void onTimeout(MediaTimer* timer) final;
private:
    PingPongKitListener* const _listener;
    // interval & timeout in milliseconds
    const uint64_t _pingInterval;
    const uint64_t _pingTimeout;
    MediaTimer _pingIntervalTimer;
    MediaTimer _pingTimeoutTimer;
};

} // namespace LiveKitCpp
