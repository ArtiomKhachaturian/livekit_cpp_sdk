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
#pragma once // MediaSourceImpl.h
#include "AsyncListeners.h"
#include "Loggable.h"
#include "MediaDeviceListener.h"
#include <api/media_stream_interface.h>
#include <atomic>

namespace LiveKitCpp
{

class AsyncMediaSourceImpl : public Bricks::LoggableS<>
{
public:
    virtual ~AsyncMediaSourceImpl() = default;
    webrtc::MediaSourceInterface::SourceState state() const noexcept { return _state; }
    const auto& signalingQueue() const noexcept { return _observers.queue(); }
    bool setEnabled(bool enabled);
    bool enabled() const { return _enabled; }
    bool active() const noexcept { return _active; }
    void close();
    void registerObserver(webrtc::ObserverInterface* observer);
    void unregisterObserver(webrtc::ObserverInterface* observer);
    void addListener(MediaDeviceListener* listener) { _listeners.add(listener); }
    void removeListener(MediaDeviceListener* listener) { _listeners.remove(listener); }
    virtual void updateAfterEnableChanges(bool /*enabled*/) {}
protected:
    AsyncMediaSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                         const std::shared_ptr<Bricks::Logger>& logger = {},
                         bool liveImmediately = false);
    void notifyAboutChanges();
    void changeState(webrtc::MediaSourceInterface::SourceState state);
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const {
        _listeners.invoke(method, std::forward<Args>(args)...);
    }
    virtual void onClosed() {}
    virtual void onMuted() {}
    virtual void onLive() {}
private:
    static constexpr auto _ended = webrtc::MediaSourceInterface::kEnded;
    AsyncListeners<webrtc::ObserverInterface*> _observers;
    Bricks::Listeners<MediaDeviceListener*> _listeners;
    std::atomic_bool _enabled = true;
    std::atomic_bool _active = true;
    std::atomic<webrtc::MediaSourceInterface::SourceState> _state;
};

} // namespace LiveKitCpp
