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
#pragma once // MFSampleAttributeQueue.h
#include <stdint.h>
#include <queue>
#include <utility>
#include "rtc_base/synchronization/mutex.h"

// A sorted queue with certain properties which makes it
// good for mapping attributes to frames and samples.
// The ids have to be in increasing order.
template <typename T>
class MFSortedQueue
{
 public:
     MFSortedQueue() = default;
     void push(uint64_t id, T t);
     bool pop(uint64_t id, T& outT);
     void clear();
     uint32_t size() const;
 private:
    mutable webrtc::Mutex _lock;
    std::queue<std::pair<uint64_t, const T>> _attributes;
};

template <typename T>
inline void MFSortedQueue<T>::push(uint64_t id, T t)
{
    const webrtc::MutexLock locker(&_lock);
    _attributes.push(std::make_pair(id, std::move(t)));
}

template <typename T>
inline bool MFSortedQueue<T>::pop(uint64_t id, T& out)
{
    const webrtc::MutexLock locker(&_lock);
    while (!_attributes.empty()) {
        auto& entry = _attributes.front();
        if (entry.first > id) {
            out = entry.second;
            return true;
        }
        if (entry.first == id) {
            out = std::move(entry.second);
            _attributes.pop();
            return true;
        }
        _attributes.pop();
    }
    return false;
}

template <typename T>
inline void MFSortedQueue<T>::clear()
{
    const webrtc::MutexLock locker(&_lock);
    while (!_attributes.empty()) {
        _attributes.pop();
    }
}

template <typename T>
inline uint32_t MFSortedQueue<T>::size() const
{
    const webrtc::MutexLock locker(&_lock);
    return static_cast<uint32_t>(_attributes.size());
}