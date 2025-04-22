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
#pragma once // VideoFrameBufferPoolSource.h
#include "SafeObj.h"
#include <api/scoped_refptr.h>
#include <api/video/i010_buffer.h>
#include <api/video/i210_buffer.h>
#include <api/video/i410_buffer.h>
#include <api/video/i420_buffer.h>
#include <api/video/i422_buffer.h>
#include <api/video/i444_buffer.h>
#include <api/video/nv12_buffer.h>
#include <list>
#include <stddef.h>

namespace LiveKitCpp
{

class RgbVideoFrameBuffer;
enum class VideoFrameType;

// concurrent & thread-safe version of webrtc::VideoFrameBufferPool
class VideoFrameBufferPoolSource : public std::enable_shared_from_this<VideoFrameBufferPoolSource>
{
    using BuffersList = std::list<webrtc::scoped_refptr<webrtc::VideoFrameBuffer>>;
public:
    static std::shared_ptr<VideoFrameBufferPoolSource> create();
    static std::shared_ptr<VideoFrameBufferPoolSource> create(size_t maxNumberOfBuffers);
    ~VideoFrameBufferPoolSource() { release(); }
    // Changes the max amount of buffers in the pool to the new value.
    // Returns true if change was successful and false if the amount of already
    // allocated buffers is bigger than new value.
    bool resize(size_t maxNumberOfBuffers);
    // Clears buffers
    void release() { _buffers({}); }
    webrtc::scoped_refptr<webrtc::I420Buffer> createI420(int width, int height);
    webrtc::scoped_refptr<webrtc::I422Buffer> createI422(int width, int height);
    webrtc::scoped_refptr<webrtc::I444Buffer> createI444(int width, int height);
    webrtc::scoped_refptr<webrtc::I010Buffer> createI010(int width, int height);
    webrtc::scoped_refptr<webrtc::I210Buffer> createI210(int width, int height);
    webrtc::scoped_refptr<webrtc::I410Buffer> createI410(int width, int height);
    webrtc::scoped_refptr<webrtc::NV12Buffer> createNV12(int width, int height);
    webrtc::scoped_refptr<RgbVideoFrameBuffer> createRgb(int width, int height,
                                                         VideoFrameType rgbFormat,
                                                         int stride = 0);
protected:
    VideoFrameBufferPoolSource();
    VideoFrameBufferPoolSource(size_t maxNumberOfBuffers);
private:
    template <typename... Args>
    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> getExisting(int width, int height,
                                                                webrtc::VideoFrameBuffer::Type type,
                                                                Args&&... args);
    template <class TBuffer, bool attachFramePool, typename... Args>
    webrtc::scoped_refptr<TBuffer> create(int width, int height, Args&&... args);
    template <typename... Args>
    static bool matched(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
                        int width, int height,
                        webrtc::VideoFrameBuffer::Type type,
                        Args&&... args);
    static bool hasOneRef(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);
private:
    Bricks::SafeObj<BuffersList> _buffers;
     // Max number of buffers this pool can have pending.
    size_t _maxNumberOfBuffers;
};
	
} // namespace LiveKitCpp
