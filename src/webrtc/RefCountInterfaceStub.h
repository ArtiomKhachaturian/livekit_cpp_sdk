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
#pragma once // RefCountInterfaceStub.h
#include <rtc_base/ref_count.h>
#include <type_traits>

namespace LiveKitCpp
{

template <class TRefCountInterface>
class RefCountInterfaceStub : public TRefCountInterface
{
    static_assert(std::is_base_of<webrtc::RefCountInterface, TRefCountInterface>::value);
public:
    // impl. of rtc::RefCountInterface
    void AddRef() const final {}
    webrtc::RefCountReleaseStatus Release() const final { return webrtc::RefCountReleaseStatus::kOtherRefsRemained; }
protected:
    template <class... Args>
    RefCountInterfaceStub(Args&&... args);
};

template <class TRefCountInterface>
template <class... Args>
inline RefCountInterfaceStub<TRefCountInterface>::RefCountInterfaceStub(Args&&... args)
    : TRefCountInterface(std::forward<Args>(args)...) {}

} // namespace LiveKitCpp
