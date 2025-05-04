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
#pragma once // VTDecoderSession.h
#include "VideoFrameBufferPool.h"
#include "VTSession.h"
#include <api/scoped_refptr.h>

namespace webrtc {
class ColorSpace;
class EncodedImageBufferInterface;
class EncodedImage;
} // namespace webrtc

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class VTDecoderSessionCallback;

class VTDecoderSession : public VTSession<CFAutoRelease<CMVideoFormatDescriptionRef>, VTDecompressionSessionRef>
{
    class DecodePipeline;
    using BaseClass = VTSession<CFAutoRelease<CMVideoFormatDescriptionRef>, VTDecompressionSessionRef>;
public:
    VTDecoderSession();
    VTDecoderSession(const VTDecoderSession&) = delete;
    VTDecoderSession(VTDecoderSession&&);
    ~VTDecoderSession();
    VTDecoderSession& operator = (const VTDecoderSession&) = delete;
    VTDecoderSession& operator = (VTDecoderSession&&);
    static webrtc::RTCErrorOr<VTDecoderSession>
        create(bool hardwareAccelerated, CMVideoCodecType codecType,
               CFAutoRelease<CMVideoFormatDescriptionRef> format,
               OSType outputPixelFormat,
               int numberOfCores = 0, // auto
               VTDecoderSessionCallback* CM_NULLABLE callback = nullptr,
               VideoFrameBufferPool framesPool = {},
               bool realtime = true);
    webrtc::RTCError waitForAsynchronousFrames();
    webrtc::RTCError setOutputPoolRequestedMinimumBufferCount(int bufferPoolSize);
    OSStatus decompress(CMSampleBufferRef CM_NONNULL encodedBufferData,
                        const webrtc::EncodedImage& image,
                        VTDecodeInfoFlags* CM_NULLABLE infoFlags = nullptr) const;
    // impl. of VTSession
    uint64_t pendingFramesCount() const final;
    OSStatus lastOutputStatus() const final;
private:
    VTDecoderSession(std::unique_ptr<DecodePipeline> pipeline,
                     CFAutoRelease<CMVideoFormatDescriptionRef> format,
                     VTDecompressionSessionRef CM_NONNULL sessionRef,
                     bool hardwareAccelerated);
    static CFDictionaryRefAutoRelease sourceImageAttributes(OSType pixelFormat,
                                                            int32_t width,
                                                            int32_t height);
private:
    std::unique_ptr<DecodePipeline> _pipeline;
};

} // namespace LiveKitCpp
