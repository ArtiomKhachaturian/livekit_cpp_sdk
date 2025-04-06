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
#include "AsyncMediaSourceImpl.h"
#include "Utils.h"

namespace LiveKitCpp
{

AsyncMediaSourceImpl::AsyncMediaSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                           const std::shared_ptr<Bricks::Logger>& logger,
                                           bool liveImmediately)
    : Bricks::LoggableS<>(logger)
    , _observers(std::move(signalingQueue))
    , _state(liveImmediately ? webrtc::MediaSourceInterface::kLive : webrtc::MediaSourceInterface::kInitializing)
{
}

void AsyncMediaSourceImpl::notifyAboutChanges()
{
    _observers.invoke(&webrtc::ObserverInterface::OnChanged);
}

void AsyncMediaSourceImpl::setEnabled(bool enabled)
{
    if (active() && enabled != _enabled.exchange(enabled)) {
        onEnabled(enabled);
    }
}

bool AsyncMediaSourceImpl::deactivate()
{
    if (_active.exchange(false)) {
        changeState(webrtc::MediaSourceInterface::kEnded);
        return true;
    }
    return false;
}

void AsyncMediaSourceImpl::close()
{
    if (!active()) {
        onClosed();
    }
}

void AsyncMediaSourceImpl::registerObserver(webrtc::ObserverInterface* observer)
{
    if (observer && active()) {
        _observers.add(observer);
    }
}

void AsyncMediaSourceImpl::unregisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.remove(observer);
}

void AsyncMediaSourceImpl::changeState(webrtc::MediaSourceInterface::SourceState state)
{
    bool changed = false;
    if (active()) {
        changed = exchangeVal(state, _state);
        if (changed) {
            switch (state) {
                case webrtc::MediaSourceInterface::kLive:
                    onLive();
                    break;
                case webrtc::MediaSourceInterface::kMuted:
                    onMuted();
                    break;
                case webrtc::MediaSourceInterface::kEnded:
                    if (_active.exchange(false)) {
                        onClosed();
                    }
                    break;
                default:
                    break;
            }
        }
    }
    else {
        changed = exchangeVal(_ended, _state);
    }
    if (changed) {
        notifyAboutChanges();
    }
}

} // namespace LiveKitCpp
