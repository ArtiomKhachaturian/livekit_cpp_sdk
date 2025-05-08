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
#pragma once // MFMediaBufferLocker.h
#include "CompletionStatusOr.h"
#include <atlbase.h> //CComPtr support
#include <memory>
#include <mfobjects.h>
#include <optional>

namespace LiveKitCpp 
{

class MFMediaBufferLocker
{
    class ImplInterface;
    template <class TMediaBuffer> class Impl;
public:
    MFMediaBufferLocker(CComPtr<IMFMediaBuffer> data, bool acquire2DBuffer = true);
    MFMediaBufferLocker(MFMediaBufferLocker&& tmp) noexcept;
    MFMediaBufferLocker(const MFMediaBufferLocker&) = delete;
    ~MFMediaBufferLocker();
    MFMediaBufferLocker& operator=(MFMediaBufferLocker&& tmp) noexcept;
    MFMediaBufferLocker& operator=(const MFMediaBufferLocker&) = delete;
    explicit operator bool() const { return ok(); }
    const auto& status() const noexcept { return _impl.status(); }
    bool ok() const;
    bool is2DBuffer() const;
    LONG pitch2D() const; // zero for non-2D buffers
    BYTE* dataBuffer() const;
    DWORD maxLen() const;
    DWORD currentLen() const;
private:
    static CompletionStatusOrUniquePtr<ImplInterface> 
        createImpl(CComPtr<IMFMediaBuffer> data, bool acquire2DBuffer);
private:
    CompletionStatusOrUniquePtr<ImplInterface> _impl;
};

} // namespace LiveKitCpp