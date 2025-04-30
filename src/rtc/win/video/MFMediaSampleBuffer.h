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
#include "MFVideoBuffer.h"
#include "NativeVideoFrameBuffer.h"
#include "VideoFrameBuffer.h"
#include <strmif.h>

namespace webrtc {
enum class VideoType;
struct VideoCaptureCapability;
} // namespace webrtc

namespace LiveKitCpp 
{

class I420BuffersPool;

class MFMediaSampleBuffer : public MFVideoBuffer<VideoFrameBuffer<NativeVideoFrameBuffer>, IMediaSample>
{
    using BaseClass = MFVideoBuffer<VideoFrameBuffer<NativeVideoFrameBuffer>, IMediaSample>;
public:
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer> create(int width, int height, 
                                                               VideoFrameType bufferType,
                                                               BYTE* buffer, DWORD actualBufferLen, 
                                                               DWORD totalBufferLen,
                                                               const CComPtr<IMediaSample>& sample,
                                                               webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                                                               const VideoFrameBufferPool& framesPool = {});
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return _bufferType;  }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
protected:
    MFMediaSampleBuffer(int width, int height, VideoFrameType bufferType,
                        BYTE* buffer, DWORD actualBufferLen, DWORD totalBufferLen,
                        const CComPtr<IMediaSample>& sample,
                        webrtc::VideoRotation rotation,
                        VideoFrameBufferPool framesPool);
    // impl. of VideoFrameBuffer<>
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
private:
    static int targetWidth(int width, int height, webrtc::VideoRotation rotation);
    static int targetHeight(int width, int height, webrtc::VideoRotation rotation);
    static void swapIfRotated(webrtc::VideoRotation rotation, int& width, int& height);
private:
    const int _originalWidth;
    const int _originalHeight;
    const webrtc::VideoRotation _rotation;
    const VideoFrameType _bufferType;
};

} // namespace LiveKitCpp