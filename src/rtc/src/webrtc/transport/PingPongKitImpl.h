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
#pragma once // PingPongKitImpl.h
#include "Listener.h"
#include "MediaTimer.h"
#include "MediaTimerCallback.h"

namespace LiveKitCpp
{

class PingPongKitListener;

class PingPongKitImpl : private MediaTimerCallback
{
public:
    // interval & timeout in seconds
    PingPongKitImpl(uint32_t pingInterval, uint32_t pingTimeout,
                    const webrtc::scoped_refptr<PeerConnectionFactory>& pcf);
    ~PingPongKitImpl() final;
    void start(PingPongKitListener* listener);
    void stop();
    void notifyThatPongReceived();
private:
    // impl. of MediaTimerCallback
    void onTimeout(uint64_t timerId) final;
private:
    // interval & timeout in milliseconds
    const uint64_t _pingInterval;
    const uint64_t _pingTimeout;
    const uint64_t _pingTimeoutTimerId;
    Bricks::Listener<PingPongKitListener*> _listener;
    MediaTimer _pingIntervalTimer;
};

} // namespace LiveKitCpp
