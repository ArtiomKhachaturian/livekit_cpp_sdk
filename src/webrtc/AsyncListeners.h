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
#include <rtc_base/thread.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template<class TListener>
class AsyncListeners
{
    using Listeners = Bricks::Listeners<TListener, true>;
    using Weak = std::weak_ptr<Listeners>;
public:
    AsyncListeners(std::weak_ptr<rtc::Thread> thread);
    AsyncListeners(AsyncListeners&&) = delete;
    AsyncListeners(const AsyncListeners&) = delete;
    ~AsyncListeners() { clear(); }
    const auto& thread() const noexcept { return _thread; }
    bool async() const noexcept { return !_thread.expired(); }
    bool add(const TListener& listener) { return _listeners->add(listener); }
    bool remove(const TListener& listener) { return _listeners->remove(listener); }
    void clear() { _listeners->clear(); }
    bool empty() const noexcept { return _listeners->empty(); }
    size_t size() const noexcept { return _listeners->size(); }
    template <class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    AsyncListeners& operator = (const AsyncListeners&) = delete;
    AsyncListeners& operator = (AsyncListeners&&) noexcept = delete;
private:
    const std::weak_ptr<rtc::Thread> _thread;
    const std::shared_ptr<Listeners> _listeners;
};


template<class TListener>
inline AsyncListeners<TListener>::AsyncListeners(std::weak_ptr<rtc::Thread> thread)
    : _thread(std::move(thread))
    , _listeners(std::make_shared<Listeners>())
{
}

template<class TListener>
template <class Method, typename... Args>
inline void AsyncListeners<TListener>::invoke(Method method, Args&&... args) const
{
    const auto thread = _thread.lock();
    if (thread && !thread->IsCurrent()) {
        thread->PostTask([method = std::move(method), // deep copy of all arguments
                          args = std::make_tuple((typename std::decay<Args>::type)args...),
                          weak = Weak(_listeners)](){
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

} // namespace LiveKitCpp
