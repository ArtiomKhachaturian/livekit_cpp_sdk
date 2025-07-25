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
#pragma once // ConditionWaiter.h
#include <condition_variable>
#include <tuple>

namespace LiveKitCpp
{

template <class... Types>
class ConditionWaiter
{
public:
    ConditionWaiter() = default;
    ConditionWaiter(Types&&... args);
    template <class... UTypes>
    ConditionWaiter(UTypes&&... args);
    template <typename T>
    void setValue(T value, bool notifyAll = true);
    template <typename T, typename TSetter>
    void set(TSetter setter, bool notifyAll = true);
    template <typename T>
    T get() const;
    template <typename T>
    const T& getRef() const;
    // return false if the predicate pred still evaluates to false after the maxTime timeout expired,
    // otherwise true.
    template <class TDuration, class Predicate>
    bool waitFor(const TDuration& maxTime, Predicate pred);
    // return false if the relative timeout specified by maxTime expired,
    // otherwise true.
    template <class TDuration>
    bool waitFor(const TDuration& maxTime);
    // simple waiting
    void wait();
    // wait until pred still evaluates to false
    template <class Predicate>
    void wait(Predicate stopWaiting);
private:
    std::tuple<Types...> _data;
    mutable std::mutex _mutex;
    std::condition_variable _signal;
};

template <class... Types>
inline ConditionWaiter<Types...>::ConditionWaiter(Types&&... args)
    : _data(std::forward<Types>(args)...)
{
}

template <class... Types>
template <class... UTypes>
inline ConditionWaiter<Types...>::ConditionWaiter(UTypes&&... args)
    : _data(std::forward<UTypes>(args)...)
{
}

template <class... Types>
template <typename T>
inline void ConditionWaiter<Types...>::setValue(T value, bool notifyAll)
{
    set<T>([value = std::move(value)]() -> T { return value; }, notifyAll);
}

template <class... Types>
template <typename T, typename TSetter>
inline void ConditionWaiter<Types...>::set(TSetter setter, bool notifyAll)
{
    {
        std::lock_guard<std::mutex> lk(_mutex);
        std::get<T>(_data) = setter();
    }
    if (notifyAll) {
        _signal.notify_all();
    } else {
        _signal.notify_one();
    }
}

template <class... Types>
template <typename T>
inline T ConditionWaiter<Types...>::get() const
{
    std::lock_guard<std::mutex> lk(_mutex);
    return getRef<T>();
}

template <class... Types>
template <typename T>
inline const T& ConditionWaiter<Types...>::getRef() const
{
    return std::get<T>(_data);
}

template <class... Types>
template <class TDuration, class Predicate>
inline bool ConditionWaiter<Types...>::waitFor(const TDuration& maxTime, Predicate pred)
{
    std::unique_lock<std::mutex> lk(_mutex);
    // wait result
    return _signal.wait_for(lk, maxTime, std::move(pred));
}

template <class... Types>
template <class TDuration>
inline bool ConditionWaiter<Types...>::waitFor(const TDuration& maxTime)
{
    std::unique_lock<std::mutex> lk(_mutex);
    // wait result
    return std::cv_status::no_timeout == _signal.wait_for(lk, maxTime);
}

template <class... Types>
inline void ConditionWaiter<Types...>::wait()
{
    std::unique_lock<std::mutex> lk(_mutex);
    // wait result
    _signal.wait(lk);
}

template <class... Types>
template <class Predicate>
inline void ConditionWaiter<Types...>::wait(Predicate stopWaiting)
{
    std::unique_lock<std::mutex> lk(_mutex);
    // wait result
    _signal.wait(lk, std::move(stopWaiting));
}

} // namespace LiveKitCpp
