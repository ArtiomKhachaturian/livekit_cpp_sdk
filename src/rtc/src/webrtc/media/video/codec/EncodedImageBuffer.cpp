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
#include "EncodedImageBuffer.h"
#include "Blob.h"
#include <api/make_ref_counted.h>

namespace
{

template<class TBuffer>
class EncodedImageBufferImpl : public LiveKitCpp::EncodedImageBuffer
{
public:
    static webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> make(TBuffer buffer);
    // impl. of webrtc::EncodedImageBufferInterface
    const uint8_t* data() const final { return _buffer.data(); }
    uint8_t* data() final { return _buffer.data(); }
    size_t size() const final { return _buffer.size(); }
protected:
    EncodedImageBufferImpl(TBuffer buffer);
private:
    TBuffer _buffer;
};

class MemoryBlockEncodedImageBuffer : public LiveKitCpp::EncodedImageBuffer
{
public:
    static webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> make(std::unique_ptr<Bricks::Blob> buffer);
    // impl. of webrtc::EncodedImageBufferInterface
    const uint8_t* data() const final { return _buffer->data(); }
    uint8_t* data() final { return _buffer->data(); }
    size_t size() const final { return _buffer->size(); }
protected:
    MemoryBlockEncodedImageBuffer(std::unique_ptr<Bricks::Blob> buffer);
private:
    const std::unique_ptr<Bricks::Blob> _buffer;
};

}

namespace LiveKitCpp
{

webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> EncodedImageBuffer::create(webrtc::Buffer buffer)
{
    return EncodedImageBufferImpl<webrtc::Buffer>::make(std::move(buffer));
}

webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> EncodedImageBuffer::create(std::vector<uint8_t> buffer)
{
    return EncodedImageBufferImpl<std::vector<uint8_t>>::make(std::move(buffer));
}

webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> EncodedImageBuffer::create(std::unique_ptr<Bricks::Blob> buffer)
{
    return MemoryBlockEncodedImageBuffer::make(std::move(buffer));
}

} // namespace LiveKitCpp

namespace
{

template<class TBuffer>
EncodedImageBufferImpl<TBuffer>::EncodedImageBufferImpl(TBuffer buffer)
    : _buffer(std::move(buffer))
{
}

template<class TBuffer>
webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> EncodedImageBufferImpl<TBuffer>::make(TBuffer buffer)
{
    return webrtc::make_ref_counted<EncodedImageBufferImpl<TBuffer>>(std::move((buffer)));
}

MemoryBlockEncodedImageBuffer::MemoryBlockEncodedImageBuffer(std::unique_ptr<Bricks::Blob> buffer)
    : _buffer(std::move(buffer))
{
}

webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> MemoryBlockEncodedImageBuffer::
    make(std::unique_ptr<Bricks::Blob> buffer)
{
    if (buffer) {
        return webrtc::make_ref_counted<MemoryBlockEncodedImageBuffer>(std::move(buffer));
    }
    return {};
}

}
