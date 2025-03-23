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
#include "PingPongKitListener.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

PingPongKit::PingPongKit(PingPongKitListener* listener,
                         uint32_t pingInterval, uint32_t pingTimeout,
                         const webrtc::scoped_refptr<PeerConnectionFactory>& pcf)
    : _listener(listener)
    , _pingInterval(pingInterval * 1000U)
    , _pingTimeout(pingTimeout * 1000U)
    , _pingIntervalTimer(pcf, this)
    , _pingTimeoutTimer(pcf, this)
{
}

PingPongKit::~PingPongKit()
{
    stop();
    _pingIntervalTimer.setCallback(nullptr);
    _pingTimeoutTimer.setCallback(nullptr);
}

void PingPongKit::start()
{
    if (_pingInterval > 0U && _listener) {
        stop();
        _pingIntervalTimer.start(_pingInterval);
    }
}

void PingPongKit::stop()
{
    if (_listener) {
        _pingIntervalTimer.stop();
        _pingTimeoutTimer.stop();
    }
}

void PingPongKit::notifyThatPongReceived()
{
    _pingTimeoutTimer.stop();
}

void PingPongKit::onTimeout(MediaTimer* timer)
{
    if (_listener) {
        if (&_pingIntervalTimer == timer) {
            _pingTimeoutTimer.stop();
            const auto ok = _listener->onPingRequested();
            if (ok && _pingTimeout > 0U) {
                _pingTimeoutTimer.start(_pingTimeout);
            }
            if (!ok) {
                _pingIntervalTimer.stop();
            }
        }
        else if (&_pingTimeoutTimer == timer) {
            stop();
            _listener->onPongTimeout();
        }
    }
}

} // namespace LiveKitCpp
