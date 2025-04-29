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
#pragma once
#include "MFVideoBufferInterface.h"
#include <cassert>
#include <atlbase.h> //CComPtr support
#include <mfobjects.h>

namespace LiveKitCpp 
{

template <class TBaseBuffer, class TMFData>
class MFVideoBuffer : public TBaseBuffer, public MFVideoBufferInterface
{
public:
    ~MFVideoBuffer() override;
    // impl. of VideoFrameBuffer
    int width() const final { return _width; }
    int height() const final { return _height; }
    // impl. of MFVideoBufferInterface
    const BYTE* buffer() const final { return _buffer; }
    DWORD actualBufferLen() const final { return _actualBufferLen; }
    DWORD totalBufferLen() const final { return _totalBufferLen; }
protected:
    template <class... Types>
    MFVideoBuffer(int width, int height,  BYTE* buffer, 
                  DWORD actualBufferLen, DWORD totalBufferLen,
                  const CComPtr<TMFData>& data, 
                  Types&&... args);
private:
    const int _width;
    const int _height;
    const CComPtr<TMFData> _data;
    BYTE* const _buffer;
    const DWORD _actualBufferLen;
    const DWORD _totalBufferLen;
};

template <class TBaseBuffer, class TMFData>
template <class... Types>
inline MFVideoBuffer<TBaseBuffer, TMFData>::MFVideoBuffer(int width, int height, 
                                                          BYTE* buffer, DWORD actualBufferLen, 
                                                          DWORD totalBufferLen,
                                                          const CComPtr<TMFData>& data, Types&&... args)
    : TBaseBuffer(std::forward<Types>(args)...)
    , _width(width)
    , _height(height)
    , _data(data)
    , _buffer(buffer)
    , _actualBufferLen(actualBufferLen)
    , _totalBufferLen(totalBufferLen)
{
    assert(_width > 0);
    assert(_height > 0);
    assert(_data);
    assert(_buffer);
    assert(_actualBufferLen > 0UL);
    assert(_totalBufferLen >= _actualBufferLen);
}

template <class TBaseBuffer, class TMFData>
inline MFVideoBuffer<TBaseBuffer, TMFData>::~MFVideoBuffer()
{
    if constexpr (std::is_same<IMFMediaBuffer, TMFData>::value) {
        _data->Unlock();
    }
}

} // namespace LiveKitCpp