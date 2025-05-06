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
    Impl(const CComPtr<TMediaBuffer>& mediaBuffer, BYTE* dataBuffer,
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

MFMediaBufferLocker::MFMediaBufferLocker(const CComPtr<IMFMediaBuffer>& data, bool acquire2DBuffer)
{
    HRESULT error = S_OK;
    _impl = createImpl(data, acquire2DBuffer, error);
    setStatus(error);
}

MFMediaBufferLocker::MFMediaBufferLocker(MFMediaBufferLocker&& tmp) noexcept
    : ComStatus(tmp.status())
    , _impl(std::move(tmp._impl))
{
    tmp.setStatus(E_POINTER);
}

MFMediaBufferLocker::~MFMediaBufferLocker()
{
}

MFMediaBufferLocker& MFMediaBufferLocker::operator=(MFMediaBufferLocker&& tmp) noexcept
{
    if (&tmp != this) {
        setStatus(tmp.status());
        _impl = std::move(tmp._impl);
        tmp.setStatus(E_POINTER);
    }
    return *this;
}

bool MFMediaBufferLocker::is2DBuffer() const
{
    return _impl && _impl->is2DBuffer();
}

LONG MFMediaBufferLocker::pitch2D() const
{
    return _impl ? _impl->pitch2D() : 0L;
}

BYTE* MFMediaBufferLocker::dataBuffer() const
{
    return _impl ? _impl->dataBuffer() : NULL;
}

DWORD MFMediaBufferLocker::maxLen() const
{
    return _impl ? _impl->maxLen() : 0UL;
}

DWORD MFMediaBufferLocker::currentLen() const
{
    return _impl ? _impl->currentLen() : 0UL;
}

void MFMediaBufferLocker::reset(bool andUnlock)
{
    if (_impl) {
        if (!andUnlock) {
            _impl->drop();
        }
        _impl.reset();
    }
}

std::unique_ptr<MFMediaBufferLocker::ImplInterface> MFMediaBufferLocker::createImpl(const CComPtr<IMFMediaBuffer>& data,
                                                                                    bool acquire2DBuffer, HRESULT& error)
{
    std::unique_ptr<ImplInterface> impl;
    if (data) {
        CComPtr<IMF2DBuffer> buffer2d;
        BYTE* dataBuffer = NULL;
        if (acquire2DBuffer) {
            // locks a video buffer that might or might not support IMF2DBuffer,
            // query for the 2-D buffer interface - OK if this fails
            if (SUCCEEDED(data->QueryInterface(IID_IMF2DBuffer, (void**)&buffer2d))) {
                LONG pitch2D = 0L;
                error = buffer2d->Lock2D(&dataBuffer, &pitch2D);
                if (SUCCEEDED(error)) {
                    impl = std::make_unique<Impl<IMF2DBuffer>>(buffer2d, dataBuffer, pitch2D);
                }
            }
        }
        if (!buffer2d) {
            DWORD maxLen = 0UL, currentLen = 0UL;
            error = data->Lock(&dataBuffer, &maxLen, &currentLen);
            if (SUCCEEDED(error)) {
                impl = std::make_unique<Impl<IMFMediaBuffer>>(data, dataBuffer, 0L, maxLen, currentLen);
            }
        }
    } else {
        error = E_POINTER;
    }
    return impl;
}

template <class TMediaBuffer>
MFMediaBufferLocker::Impl<TMediaBuffer>::Impl(const CComPtr<TMediaBuffer>& mediaBuffer, BYTE* dataBuffer,
                                              LONG pitch2D, DWORD maxLen, DWORD currentLen)
    : _mediaBuffer(mediaBuffer)
    , _dataBuffer(dataBuffer)
    , _pitch2D(pitch2D)
    , _maxLen(maxLen)
    , _currentLen(currentLen)
{
}

template <class TMediaBuffer>
MFMediaBufferLocker::Impl<TMediaBuffer>::~Impl()
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
void MFMediaBufferLocker::Impl<TMediaBuffer>::drop()
{
    if (_mediaBuffer) {
        _mediaBuffer = CComPtr<TMediaBuffer>();
        _dataBuffer = NULL;
        _pitch2D = 0L;
        _maxLen = _currentLen = 0UL;
    }
}

} // namespace LiveKitCpp