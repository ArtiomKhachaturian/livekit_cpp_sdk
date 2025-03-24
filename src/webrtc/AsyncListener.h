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
#include <rtc_base/thread.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template<class TListener, bool forcePost = false>
class AsyncListener
{
    using Listener = Bricks::Listener<TListener>;
public:
    AsyncListener(const std::weak_ptr<rtc::Thread>& thread);
    AsyncListener(AsyncListener&&) = delete;
    AsyncListener(const AsyncListener&) = delete;
    ~AsyncListener() { reset(); }
    const auto& thread() const noexcept { return _thread; }
    bool async() const noexcept { return !_thread.expired(); }
    template <typename U = TListener>
    void set(U listener = {}) { _listener->set(std::move(listener)); }
    void reset() { _listener->reset(); }
    bool empty() const noexcept { return _listener->empty(); }
    template <class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    AsyncListener& operator = (const AsyncListener&) = delete;
    AsyncListener& operator = (AsyncListener&&) noexcept = delete;
private:
    const std::weak_ptr<rtc::Thread> _thread;
    const std::shared_ptr<Listener> _listener;
};

template<class TListener, bool forcePost>
inline AsyncListener<TListener, forcePost>::
    AsyncListener(const std::weak_ptr<rtc::Thread>& thread)
    : _thread(thread)
    , _listener(std::make_shared<Listener>())
{
}

template<class TListener, bool forcePost>
template <class Method, typename... Args>
inline void AsyncListener<TListener, forcePost>::
    invoke(Method method, Args&&... args) const
{
    if (const auto thread = _thread.lock()) {
        if (forcePost || !thread->IsCurrent()) {
            using WeakRef = std::weak_ptr<Listener>;
            thread->PostTask([method = std::move(method), // deep copy of all arguments
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

} // namespace LiveKitCpp
