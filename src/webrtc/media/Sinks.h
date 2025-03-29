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
#pragma once // Sinks.h
#include "Listeners.h"

namespace LiveKitCpp
{

template<class TSink, class TRtcSink>
class Sinks : public TRtcSink
{
public:
    bool clear() { return _sinks.clear(); }
    bool empty() const noexcept { return _sinks.empty(); }
    size_t size() const noexcept { return _sinks.size(); }
    Bricks::AddResult add(TSink* sink);
    Bricks::RemoveResult remove(TSink* sink);
protected:
    Sinks() = default;
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const;
private:
    Bricks::Listeners<TSink*> _sinks;
};

template<class TSink, class TRtcSink>
inline Bricks::AddResult Sinks<TSink, TRtcSink>::add(TSink* sink)
{
    return _sinks.add(sink);
}

template<class TSink, class TRtcSink>
inline Bricks::RemoveResult Sinks<TSink, TRtcSink>::remove(TSink* sink)
{
    return _sinks.remove(sink);
}

template<class TSink, class TRtcSink>
template <class Method, typename... Args>
inline void Sinks<TSink, TRtcSink>::invoke(const Method& method, Args&&... args) const
{
    _sinks.invoke(method, std::forward<Args>(args)...);
}

} // namespace LiveKitCpp
