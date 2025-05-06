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
#pragma once // VTDecoder.h
#include "CFAutoRelease.h"
#include "CodecStatus.h"
#include "SafeObj.h"
#include "VideoDecoder.h"
#include "VideoUtils.h"
#include "VTDecoderSessionCallback.h"
#include "VTDecoderSession.h"

namespace LiveKitCpp
{

class CFMemoryPool;
class VideoFrameBufferPoolSource;

class VTDecoder : public VideoDecoder, private VTDecoderSessionCallback
{
public:
    ~VTDecoder() override;
    // overrides of GenericCodec<>
    bool hardwareAccelerated() const override;
    // overrides of VideoDecoder
    bool Configure(const Settings& settings) override;
    int32_t Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) final;
protected:
    VTDecoder(const webrtc::SdpVideoFormat& format,
              const std::shared_ptr<CFMemoryPool>& memoryPool = {},
              OSType outputPixelFormat = formatNV12Full());
    const auto& memoryPool() const noexcept { return _memoryPool; }
    CMVideoFormatDescriptionRef createInitialVideoFormat(const webrtc::RenderResolution& resolution) const;
    virtual CMVideoFormatDescriptionRef createInitialVideoFormat(uint32_t encodedWidth, uint32_t encodedHeight) const;
    virtual CMVideoFormatDescriptionRef createVideoFormat(const webrtc::EncodedImage& inputImage) const = 0;
    virtual CMSampleBufferRef createSampleBuffer(const webrtc::EncodedImage& inputImage, CMVideoFormatDescriptionRef format) const = 0;
    // impl. of VideoDecoder
    void destroySession() override;
private:
    // takes ownership to format
    CompletionStatus createSession(CFAutoRelease<CMVideoFormatDescriptionRef> format, bool realtime = true);
    // impl. of VTDecoderSessionCallback
    void onDecodedImage(CMTime timestamp, CMTime duration,
                        VTDecodeInfoFlags infoFlags,
                        uint32_t encodedImageTimestamp,
                        webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                        std::optional<uint8_t> qp,
                        std::optional<webrtc::ColorSpace> encodedImageColorspace) final;
    void onError(CompletionStatus error, bool fatal) final;
protected:
    const OSType _outputPixelFormat;
    const std::shared_ptr<CFMemoryPool> _memoryPool;
    const std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    VTDecoderSession _session;
    std::atomic<int> _numberOfCores = 0;
};

} // namespace LiveKitCpp
