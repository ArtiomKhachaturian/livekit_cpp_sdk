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
#include "VideoFrameBufferPoolSource.h"
#include <api/make_ref_counted.h>
#include <limits>

namespace
{

template <class TBuffer>
inline bool testOneRef(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer) {
    return static_cast<webrtc::RefCountedObject<TBuffer>*>(buffer.get())->HasOneRef();
}

template <class TBuffer>
inline webrtc::VideoFrameBuffer::Type type() { return webrtc::VideoFrameBuffer::Type::kNative; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I420Buffer>() { return webrtc::VideoFrameBuffer::Type::kI420; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I422Buffer>() { return webrtc::VideoFrameBuffer::Type::kI422; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I444Buffer>() { return webrtc::VideoFrameBuffer::Type::kI444; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I010Buffer>() { return webrtc::VideoFrameBuffer::Type::kI010; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I210Buffer>() { return webrtc::VideoFrameBuffer::Type::kI210; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::I410Buffer>() { return webrtc::VideoFrameBuffer::Type::kI410; }

template <>
inline webrtc::VideoFrameBuffer::Type type<webrtc::NV12Buffer>() { return webrtc::VideoFrameBuffer::Type::kNV12; }

}

namespace LiveKitCpp
{

VideoFrameBufferPoolSource::VideoFrameBufferPoolSource()
    : VideoFrameBufferPoolSource(std::numeric_limits<size_t>::max())
{
}

VideoFrameBufferPoolSource::VideoFrameBufferPoolSource(size_t maxNumberOfBuffers)
    : _maxNumberOfBuffers(maxNumberOfBuffers)
{
}

bool VideoFrameBufferPoolSource::resize(size_t maxNumberOfBuffers)
{
    LOCK_WRITE_SAFE_OBJ(_buffers);
    if (maxNumberOfBuffers != _maxNumberOfBuffers) {
        size_t usedBuffersCount = 0U;
        for (const auto& buffer : _buffers.constRef()) {
            // If the buffer is in use, the ref count will be >= 2, one from the list we
            // are looping over and one from the application. If the ref count is 1,
            // then the list we are looping over holds the only reference and it's safe
            // to reuse.
            if (!hasOneRef(buffer)) {
                usedBuffersCount++;
            }
        }
        if (usedBuffersCount > maxNumberOfBuffers) {
            return false;
        }
        _maxNumberOfBuffers = maxNumberOfBuffers;
        size_t buffersToPurge = _buffers->size() - _maxNumberOfBuffers;
        auto it = _buffers->begin();
        while (it != _buffers->end() && buffersToPurge > 0U) {
            if (hasOneRef(*it)) {
                it = _buffers->erase(it);
                buffersToPurge--;
            } else {
                ++it;
            }
        }
    }
    return true;
}

webrtc::scoped_refptr<webrtc::I420Buffer> VideoFrameBufferPoolSource::createI420(int width, int height)
{
    return create<webrtc::I420Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::I422Buffer> VideoFrameBufferPoolSource::createI422(int width, int height)
{
    return create<webrtc::I422Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::I444Buffer> VideoFrameBufferPoolSource::createI444(int width, int height)
{
    return create<webrtc::I444Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::I010Buffer> VideoFrameBufferPoolSource::createI010(int width, int height)
{
    return create<webrtc::I010Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::I210Buffer> VideoFrameBufferPoolSource::createI210(int width, int height)
{
    return create<webrtc::I210Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::I410Buffer> VideoFrameBufferPoolSource::createI410(int width, int height)
{
    return create<webrtc::I410Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::NV12Buffer> VideoFrameBufferPoolSource::createNV12(int width, int height)
{
    return create<webrtc::NV12Buffer>(width, height);
}

webrtc::scoped_refptr<webrtc::VideoFrameBuffer> VideoFrameBufferPoolSource::
    getExisting(int width, int height, webrtc::VideoFrameBuffer::Type type)
{
    // Release buffers with wrong resolution or different type.
    for (auto it = _buffers->begin(); it != _buffers->end();) {
        const auto& buffer = *it;
        if (buffer->width() != width || buffer->height() != height || buffer->type() != type) {
            it = _buffers->erase(it);
        } else {
            ++it;
        }
    }
    // Look for a free buffer.
    for (const auto& buffer : _buffers.constRef()) {
        // If the buffer is in use, the ref count will be >= 2, one from the list we
        // are looping over and one from the application. If the ref count is 1,
        // then the list we are looping over holds the only reference and it's safe
        // to reuse.
        if (hasOneRef(buffer)) {
            RTC_CHECK(buffer->type() == type);
            return buffer;
        }
    }
    return {};
}

template <class TBuffer>
webrtc::scoped_refptr<TBuffer> VideoFrameBufferPoolSource::create(int width, int height)
{
    if (width > 0 && height > 0) {
        LOCK_WRITE_SAFE_OBJ(_buffers);
        if (auto buffer = getExisting(width, height, type<TBuffer>())) {
            // Cast is safe because the only way kI420 buffer is created is
            // in the same function below, where `RefCountedObject<I420Buffer>` is
            // created.
            auto rawBuffer = static_cast<webrtc::RefCountedObject<TBuffer>*>(buffer.get());
            // Creates a new scoped_refptr, which is also pointing to the same
            // RefCountedObject as buffer, increasing ref count.
            return webrtc::scoped_refptr<TBuffer>(rawBuffer);
        }
        if (_buffers->size() >= _maxNumberOfBuffers) {
            return nullptr;
        }
        // Allocate new buffer.
        auto buffer = TBuffer::Create(width, height);
        _buffers->push_back(buffer);
        return buffer;
    }
    return {};
}

bool VideoFrameBufferPoolSource::hasOneRef(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
{
    if (buffer) {
        // Cast to RefCountedObject is safe because this function is only called
        // on locally created VideoFrameBuffers, which are either
        // `RefCountedObject<I420Buffer>`, `RefCountedObject<I444Buffer>` or
        // `RefCountedObject<NV12Buffer>`.
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kI420:
                return testOneRef<webrtc::I420Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kI444:
                return testOneRef<webrtc::I444Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kI422:
                return testOneRef<webrtc::I422Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kI010:
                return testOneRef<webrtc::I010Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kI210:
                return testOneRef<webrtc::I210Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kI410:
                return testOneRef<webrtc::I410Buffer>(buffer);
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return testOneRef<webrtc::NV12Buffer>(buffer);
            default:
                RTC_DCHECK_NOTREACHED();
        }
    }
    return false;
}

} // namespace LiveKitCpp
