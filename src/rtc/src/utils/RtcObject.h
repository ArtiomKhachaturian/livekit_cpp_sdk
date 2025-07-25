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
#pragma once // RtcObject.h
#include <memory>
#include <cstdint>

namespace LiveKitCpp
{

template<class TImpl, class... BaseInterfaces>
class RtcObject :  public BaseInterfaces...
{
public:
    ~RtcObject() { dispose(); }
    uint64_t id() const noexcept { return reinterpret_cast<uint64_t>(this); }
protected:
    template <class... Args>
    RtcObject(Args&&... args);
    RtcObject(std::shared_ptr<TImpl> impl);
    std::shared_ptr<TImpl> loadImpl() const { return std::atomic_load(&_impl); }
    std::shared_ptr<TImpl> dispose() { return std::atomic_exchange(&_impl, std::shared_ptr<TImpl>{}); }
private:
    std::shared_ptr<TImpl> _impl;
};

template<class TImpl, class... BaseInterfaces>
template <class... Args>
inline RtcObject<TImpl, BaseInterfaces...>::RtcObject(Args&&... args)
    : RtcObject(std::make_shared<TImpl>(std::forward<Args>(args)...))
{
}

template<class TImpl, class... BaseInterfaces>
inline RtcObject<TImpl, BaseInterfaces...>::RtcObject(std::shared_ptr<TImpl> impl)
    : _impl(std::move(impl))
{
}
	
} // namespace LiveKitCpp
