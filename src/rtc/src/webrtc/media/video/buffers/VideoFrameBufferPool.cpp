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
#include "VideoFrameBufferPool.h"
#include "VideoFrameBufferPoolSource.h"
#include "RgbVideoFrameBuffer.h"
#include "VideoUtils.h"

namespace LiveKitCpp
{

VideoFrameBufferPool::VideoFrameBufferPool(std::weak_ptr<VideoFrameBufferPoolSource> source)
    : _source(std::move(source))
{
}

VideoContentHint VideoFrameBufferPool::contentHint() const
{
    if (const auto source = _source.lock()) {
        return source->contentHint();
    }
    return VideoContentHint::None;
}

webrtc::scoped_refptr<webrtc::I420Buffer> VideoFrameBufferPool::createI420(int width, int height) const
{
    return create<webrtc::I420Buffer>(&VideoFrameBufferPoolSource::createI420, width, height);
}

webrtc::scoped_refptr<webrtc::I422Buffer> VideoFrameBufferPool::createI422(int width, int height) const
{
    return create<webrtc::I422Buffer>(&VideoFrameBufferPoolSource::createI422, width, height);
}

webrtc::scoped_refptr<webrtc::I444Buffer> VideoFrameBufferPool::createI444(int width, int height) const
{
    return create<webrtc::I444Buffer>(&VideoFrameBufferPoolSource::createI444, width, height);
}

webrtc::scoped_refptr<webrtc::I010Buffer> VideoFrameBufferPool::createI010(int width, int height) const
{
    return create<webrtc::I010Buffer>(&VideoFrameBufferPoolSource::createI010, width, height);
}

webrtc::scoped_refptr<webrtc::I210Buffer> VideoFrameBufferPool::createI210(int width, int height) const
{
    return create<webrtc::I210Buffer>(&VideoFrameBufferPoolSource::createI210, width, height);
}

webrtc::scoped_refptr<webrtc::I410Buffer> VideoFrameBufferPool::createI410(int width, int height) const
{
    return create<webrtc::I410Buffer>(&VideoFrameBufferPoolSource::createI410, width, height);
}

webrtc::scoped_refptr<webrtc::NV12Buffer> VideoFrameBufferPool::createNV12(int width, int height) const
{
    return create<webrtc::NV12Buffer>(&VideoFrameBufferPoolSource::createNV12, width, height);
}

webrtc::scoped_refptr<RgbVideoFrameBuffer> VideoFrameBufferPool::createRgb(int width, int height,
                                                                           VideoFrameType rgbFormat,
                                                                           int stride) const
{
    return create<RgbVideoFrameBuffer>(&VideoFrameBufferPoolSource::createRgb,
                                       width, height, rgbFormat, stride);
}

template <class TBuffer, class TMethod, typename... Args>
webrtc::scoped_refptr<TBuffer> VideoFrameBufferPool::create(TMethod method,
                                                            int width, int height,
                                                            Args&&... args) const
{
    webrtc::scoped_refptr<TBuffer> buffer;
    if (width > 0 && height > 0) {
        if (const auto source = _source.lock()) {
            buffer = ((*source).*method)(width, height, std::forward<Args>(args)...);
        }
        if (!buffer) {
            buffer = TBuffer::Create(width, height, std::forward<Args>(args)...);
        }
    }
    return buffer;
}

} // namespace LiveKitCpp
