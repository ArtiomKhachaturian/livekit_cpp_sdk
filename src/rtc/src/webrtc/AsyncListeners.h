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
#pragma once // AsyncListeners.h
#include "Listeners.h"
#include "PeerConnectionFactory.h"
#include <api/task_queue/task_queue_base.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template <class T, bool forcePost = false>
class AsyncListeners
{
    using Listeners = Bricks::Listeners<T, true>;
public:
    AsyncListeners(std::weak_ptr<webrtc::TaskQueueBase> queue);
    AsyncListeners(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf);
    AsyncListeners(AsyncListeners&&) = delete;
    AsyncListeners(const AsyncListeners&) = delete;
    ~AsyncListeners() { clear(); }
    const auto& queue() const noexcept { return _queue; }
    bool async() const noexcept { return !queue().expired(); }
    Bricks::AddResult add(const T& listener);
    Bricks::RemoveResult remove(const T& listener);
    void clear() { _listeners->clear(); }
    bool empty() const noexcept { return _listeners->empty(); }
    size_t size() const noexcept { return _listeners->size(); }
    bool contains(const T& listener) const;
    template <class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    template <class Functor>
    void apply(Functor functor) const;
    AsyncListeners& operator = (const AsyncListeners&) = delete;
    AsyncListeners& operator = (AsyncListeners&&) noexcept = delete;
    explicit operator bool() const noexcept { return !_listeners->empty(); }
private:
    const std::weak_ptr<webrtc::TaskQueueBase> _queue;
    const std::shared_ptr<Listeners> _listeners;
};

template <class T, bool forcePost>
inline AsyncListeners<T, forcePost>::
    AsyncListeners(std::weak_ptr<webrtc::TaskQueueBase> queue)
        : _queue(std::move(queue))
        , _listeners(std::make_shared<Listeners>())
{
}

template <class T, bool forcePost>
inline AsyncListeners<T, forcePost>::
    AsyncListeners(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf)
        : AsyncListeners(pcf ? pcf->eventsQueue() : std::weak_ptr<webrtc::TaskQueueBase>())
{
}

template <class T, bool forcePost>
inline Bricks::AddResult AsyncListeners<T, forcePost>::
    add(const T& listener)
{
    return _listeners->add(listener);
}

template <class T, bool forcePost>
inline Bricks::RemoveResult AsyncListeners<T, forcePost>::
    remove(const T& listener)
{
    return _listeners->remove(listener);
}

template <class T, bool forcePost>
inline bool AsyncListeners<T, forcePost>::contains(const T& listener) const
{
    return _listeners->contains(listener);
}

template <class T, bool forcePost>
template <class Method, typename... Args>
inline void AsyncListeners<T, forcePost>::
    invoke(Method method, Args&&... args) const
{
    if (const auto queue = _queue.lock()) {
        if (forcePost || !queue->IsCurrent()) {
            using WeakRef = std::weak_ptr<Listeners>;
            queue->PostTask([method = std::move(method), // deep copy of all arguments
                             args = std::make_tuple((typename std::decay<Args>::type)args...),
                             weak = WeakRef(_listeners)](){
                if (const auto strong = weak.lock()) {
                    std::apply([&strong, &method](auto&&... args) {
                        strong->invoke(method, std::forward<decltype(args)>(args)...);
                    }, args);
                }
            });
        }
        else {
            _listeners->invoke(method, std::forward<Args>(args)...);
        }
    }
}

template <class T, bool forcePost>
template <class Functor>
inline void AsyncListeners<T, forcePost>::apply(Functor functor) const
{
    if (const auto queue = _queue.lock()) {
        if (forcePost || !queue->IsCurrent()) {
            using WeakRef = std::weak_ptr<Listeners>;
            queue->PostTask([functor = std::move(functor), weak = WeakRef(_listeners)]() {
                if (const auto strong = weak.lock()) {
                    strong->apply(functor);
                }
            });
        }
        else {
            _listeners->apply(functor);
        }
    }
}

} // namespace LiveKitCpp
