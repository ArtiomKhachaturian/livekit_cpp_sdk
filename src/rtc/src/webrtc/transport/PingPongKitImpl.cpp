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
#include "PingPongKitImpl.h"
#include "PingPongKitListener.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

PingPongKitImpl::PingPongKitImpl(uint32_t pingInterval, uint32_t pingTimeout,
                                 const webrtc::scoped_refptr<PeerConnectionFactory>& pcf)
    : _pingInterval(pingInterval * 1000U)
    , _pingTimeout(pingTimeout * 1000U)
    , _pingIntervalTimer(pcf, this)
    , _pingTimeoutTimerId(reinterpret_cast<uint64_t>(this))
{
}

PingPongKitImpl::~PingPongKitImpl()
{
    stop();
    _pingIntervalTimer.setCallback(nullptr);
}

void PingPongKitImpl::start(PingPongKitListener* listener)
{
    if (_pingInterval > 0U && listener) {
        stop();
        _listener = listener;
        _pingIntervalTimer.start(_pingInterval);
    }
}

void PingPongKitImpl::stop()
{
    if (_listener) {
        _listener.reset();
        _pingIntervalTimer.stop();
        _pingIntervalTimer.cancelSingleShot(_pingTimeoutTimerId);
    }
}

void PingPongKitImpl::notifyThatPongReceived()
{
    _pingIntervalTimer.cancelSingleShot(_pingTimeoutTimerId);
}

void PingPongKitImpl::onTimeout(uint64_t timerId)
{
    if (_pingIntervalTimer.id() == timerId) {
        _pingIntervalTimer.cancelSingleShot(_pingTimeoutTimerId);
        const auto ok = _listener.invokeR<bool>(&PingPongKitListener::onPingRequested);
        if (ok && _pingTimeout > 0U) {
            _pingIntervalTimer.singleShot(this, _pingTimeout, _pingTimeoutTimerId);
        }
        if (!ok) {
            _pingIntervalTimer.stop();
        }
    }
    else if (_pingTimeoutTimerId == timerId) {
        _pingIntervalTimer.stop();
        _listener.invoke(&PingPongKitListener::onPongTimeout);
    }
}

} // namespace LiveKitCpp
