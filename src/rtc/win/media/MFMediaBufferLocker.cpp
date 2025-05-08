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
#include "MFMediaBufferLocker.h"
#include <type_traits>

namespace LiveKitCpp 
{

class MFMediaBufferLocker::ImplInterface
{
public:
    virtual ~ImplInterface() = default;
    virtual void drop() = 0;
    virtual BYTE* dataBuffer() const = 0;
    virtual bool is2DBuffer() const = 0;
    virtual LONG pitch2D() const { return 0L; }
    virtual DWORD maxLen() const { return 0UL; }
    virtual DWORD currentLen() const { return 0UL; }
};

template <class TMediaBuffer>
class MFMediaBufferLocker::Impl : public MFMediaBufferLocker::ImplInterface
{
public:
    Impl(CComPtr<TMediaBuffer> mediaBuffer, BYTE* dataBuffer,
         LONG pitch2D = 0L, DWORD maxLen = 0UL, DWORD currentLen = 0UL);
    ~Impl() final;
    // impl. of ImplInterface
    void drop() final;
    BYTE* dataBuffer() const final { return _dataBuffer; }
    bool is2DBuffer() const final { return std::is_same<IMF2DBuffer, TMediaBuffer>::value; }
    LONG pitch2D() const final { return _pitch2D; }
    DWORD maxLen() const final { return _maxLen; }
    DWORD currentLen() const final { return _currentLen; }
private:
    CComPtr<TMediaBuffer> _mediaBuffer;
    BYTE* _dataBuffer;
    LONG _pitch2D;
    DWORD _maxLen;
    DWORD _currentLen;
};

MFMediaBufferLocker::MFMediaBufferLocker(const CComPtr<IMFMediaBuffer>& data, 
                                         bool acquire2DBuffer)
    : _impl(createImpl(data, acquire2DBuffer))
{
}

MFMediaBufferLocker::MFMediaBufferLocker(MFMediaBufferLocker&& tmp) noexcept
    : _impl(std::move(tmp._impl))
{
}

MFMediaBufferLocker::~MFMediaBufferLocker()
{
}

MFMediaBufferLocker& MFMediaBufferLocker::operator=(MFMediaBufferLocker&& tmp) noexcept
{
    if (&tmp != this) {
        _impl = std::move(tmp._impl);
    }
    return *this;
}

bool MFMediaBufferLocker::ok() const
{
    return _impl && nullptr != _impl->get();
}

bool MFMediaBufferLocker::is2DBuffer() const
{
    const auto& impl = _impl.value();
    return impl && impl->is2DBuffer();
}

LONG MFMediaBufferLocker::pitch2D() const
{
    const auto& impl = _impl.value();
    return impl ? impl->pitch2D() : 0L;
}

BYTE* MFMediaBufferLocker::dataBuffer() const
{
    const auto& impl = _impl.value();
    return impl ? impl->dataBuffer() : NULL;
}

DWORD MFMediaBufferLocker::maxLen() const
{
    const auto& impl = _impl.value();
    return impl ? impl->maxLen() : 0UL;
}

DWORD MFMediaBufferLocker::currentLen() const
{
    const auto& impl = _impl.value();
    return impl ? impl->currentLen() : 0UL;
}

void MFMediaBufferLocker::reset(bool andUnlock)
{
    if (auto& impl = _impl.value()) {
        if (!andUnlock) {
            impl->drop();
        }
        impl.reset();
    }
}

CompletionStatusOrUniquePtr<MFMediaBufferLocker::ImplInterface> MFMediaBufferLocker::
    createImpl(const CComPtr<IMFMediaBuffer>& data, bool acquire2DBuffer)
{
    if (!data) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    std::unique_ptr<ImplInterface> impl;
    BYTE* dataBuffer = NULL;
    if (acquire2DBuffer) {
        CComPtr<IMF2DBuffer> buffer2d;
        // locks a video buffer that might or might not support IMF2DBuffer,
        // query for the 2-D buffer interface - OK if this fails
        auto status = COMPLETION_STATUS(data->QueryInterface(IID_IMF2DBuffer, (void**)&buffer2d));
        if (!status) {
            return status;
        }
        LONG pitch2D = 0L;
        status = COMPLETION_STATUS(buffer2d->Lock2D(&dataBuffer, &pitch2D));
        if (!status) {
            return status;
        }
        impl.reset(new Impl<IMF2DBuffer>(buffer2d, dataBuffer, pitch2D));
    }
    else {
        DWORD maxLen = 0UL, currentLen = 0UL;
        auto status = COMPLETION_STATUS(data->Lock(&dataBuffer, &maxLen, &currentLen));
        if (!status) {
            return status;
        }
        impl.reset(new Impl<IMFMediaBuffer>(data, dataBuffer, 0L, maxLen, currentLen));
    }
    return impl;
}

template <class TMediaBuffer>
inline MFMediaBufferLocker::Impl<TMediaBuffer>::Impl(CComPtr<TMediaBuffer> mediaBuffer, 
                                                     BYTE* dataBuffer,
                                                     LONG pitch2D, DWORD maxLen, 
                                                     DWORD currentLen)
    : _mediaBuffer(std::move(mediaBuffer))
    , _dataBuffer(dataBuffer)
    , _pitch2D(pitch2D)
    , _maxLen(maxLen)
    , _currentLen(currentLen)
{
}

template <class TMediaBuffer>
inline MFMediaBufferLocker::Impl<TMediaBuffer>::~Impl()
{
    if (_mediaBuffer) {
        if constexpr (std::is_same<IMF2DBuffer, TMediaBuffer>::value) {
            _mediaBuffer->Unlock2D();
        } else if constexpr (std::is_same<IMFMediaBuffer, TMediaBuffer>::value) {
            _mediaBuffer->Unlock();
        } else {
            static_assert(false);
        }
    }
}

template <class TMediaBuffer>
inline void MFMediaBufferLocker::Impl<TMediaBuffer>::drop()
{
    if (_mediaBuffer) {
        _mediaBuffer = {};
        _dataBuffer = NULL;
        _pitch2D = 0L;
        _maxLen = _currentLen = 0UL;
    }
}

} // namespace LiveKitCpp