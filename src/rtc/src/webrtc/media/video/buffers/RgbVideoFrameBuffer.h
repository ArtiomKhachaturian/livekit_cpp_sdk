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
#pragma once // RgbVideoFrameBuffer.h
#include "RgbGenericVideoFrameBuffer.h"
#include <memory>

namespace LiveKitCpp
{

class RgbVideoFrameBuffer : public RgbGenericVideoFrameBuffer
{
public:
    // method name started from capital letter for compatibility with WebRTC API
    static rtc::scoped_refptr<RgbVideoFrameBuffer> Create(int width, int height,
                                                          VideoFrameType rgbFormat,
                                                          int stride = 0,
                                                          VideoFrameBufferPool framesPool = {});
    // impl. of NativeVideoFrameBuffer
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    // impl. of webrtc::VideoFrameBuffer
    int width() const final { return _width; }
    int height() const final { return _height; }
    std::string storage_representation() const final;
protected:
    RgbVideoFrameBuffer(VideoFrameType rgbFormat,
                        int width, int height, int stride = 0,
                        VideoFrameBufferPool framesPool = {});
private:
    static std::unique_ptr<std::byte[]> allocate(int stride, int height);
private:
    const int _width;
    const int _height;
    const int _stride;
    const std::unique_ptr<std::byte[]> _data;
};

} // namespace LiveKitCpp
