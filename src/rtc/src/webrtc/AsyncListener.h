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
#pragma once // AsyncListener.h
#include "Listener.h"
#include "PeerConnectionFactory.h"
#include <api/task_queue/task_queue_base.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template <class T, bool forcePost = false>
class AsyncListener
{
    using Listener = Bricks::Listener<T>;
public:
    AsyncListener(std::weak_ptr<webrtc::TaskQueueBase> queue);
    AsyncListener(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf);
    AsyncListener(AsyncListener&&) = delete;
    AsyncListener(const AsyncListener&) = delete;
    ~AsyncListener() { reset(); }
    const auto& queue() const noexcept { return _queue; }
    bool async() const noexcept { return !queue().expired(); }
    template <typename U = T>
    void set(U listener = {}) { _listener->set(std::move(listener)); }
    void reset() { _listener->reset(); }
    bool empty() const noexcept { return _listener->empty(); }
    template <class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    AsyncListener& operator = (const AsyncListener&) = delete;
    AsyncListener& operator = (AsyncListener&&) noexcept = delete;
    template <typename U = T>
    AsyncListener& operator=(U listener) noexcept;
    explicit operator bool() const noexcept { return !_listener->empty(); }
    // sync operation in the caller thread
    template <typename R, class Method, typename... Args>
    R invokeR(const Method& method, Args&&... args) const;
private:
    const std::weak_ptr<webrtc::TaskQueueBase> _queue;
    const std::shared_ptr<Listener> _listener;
};

template <class T, bool forcePost>
inline AsyncListener<T, forcePost>::
    AsyncListener(std::weak_ptr<webrtc::TaskQueueBase> queue)
        : _queue(std::move(queue))
        , _listener(std::make_shared<Listener>())
{
}

template <class T, bool forcePost>
inline AsyncListener<T, forcePost>::
    AsyncListener(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf)
        : AsyncListener(pcf ? pcf->eventsQueue() : std::weak_ptr<webrtc::TaskQueueBase>())
{
}

template <class T, bool forcePost>
template <class Method, typename... Args>
inline void AsyncListener<T, forcePost>::
    invoke(Method method, Args&&... args) const
{
    if (const auto queue = _queue.lock()) {
        if (forcePost || !queue->IsCurrent()) {
            using WeakRef = std::weak_ptr<Listener>;
            queue->PostTask([method = std::move(method), // deep copy of all arguments
                             args = std::make_tuple((typename std::decay<Args>::type)args...),
                             weak = WeakRef(_listener)](){
                if (const auto strong = weak.lock()) {
                    std::apply([&strong, &method](auto&&... args) {
                        strong->invoke(method, std::forward<decltype(args)>(args)...);
                    }, args);
                }
            });
        }
        else {
            _listener->invoke(method, std::forward<Args>(args)...);
        }
    }
}

template <class T, bool forcePost>
template <typename U>
inline AsyncListener<T, forcePost>& AsyncListener<T, forcePost>::
    operator=(U listener) noexcept
{
    set(std::move(listener));
    return *this;
}

template <class T, bool forcePost>
template <typename R, class Method, typename... Args>
inline R AsyncListener<T, forcePost>::invokeR(const Method& method, Args&&... args) const
{
    
}

} // namespace LiveKitCpp
