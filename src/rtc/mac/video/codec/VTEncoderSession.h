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
#pragma once // VTEncoderSession.h
#include "VTSession.h"
#include "VTEncoderSourceFrame.h"

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class CFMemoryPool;
class VTEncoderSessionCallback;

class VTEncoderSession : public VTSession<OSType, VTCompressionSessionRef>
{
    class EncodePipeline;
public:
    VTEncoderSession();
    VTEncoderSession(const VTEncoderSession&) = delete;
    VTEncoderSession(VTEncoderSession&&);
    ~VTEncoderSession();
    VTEncoderSession& operator = (const VTEncoderSession&) = delete;
    VTEncoderSession& operator = (VTEncoderSession&&);
    webrtc::RTCError setExpectedFrameRate(uint32_t frameRate);
    webrtc::RTCError setDataRateLimits(uint32_t maxBitrateBps);
    webrtc::RTCError setAverageBitRate(uint32_t bitrateBps);
    webrtc::RTCError setRealTime(bool realtime);
    webrtc::RTCError setAllowFrameReordering(bool allow);
    webrtc::RTCError setAllowTemporalCompression(bool allow);
    webrtc::RTCError setMaxKeyFrameInterval(int maxKeyFrameInterval);
    webrtc::RTCError setMaxKeyFrameIntervalDuration(int maxKeyFrameIntervalDuration);
    webrtc::RTCError setProfileLevel(CFStringRef CM_NONNULL profile);
    webrtc::RTCError prepareToEncodeFrames();
    webrtc::RTCError completeFrames(CMTime completeUntilPresentationTimeStamp = kCMTimeInvalid);
    int32_t width() const { return _width; }
    int32_t height() const { return _height; }
    bool isCompatible(const VTEncoderSourceFrame& frame) const;
    OSStatus compress(VTEncoderSourceFrame sourceFrame, bool forceKeyFrame,
                      VTEncodeInfoFlags* CM_NULLABLE infoFlagsOut = nullptr) const;
    // impl. of VTSession
    uint64_t pendingFramesCount() const final;
    OSStatus lastOutputStatus() const final;
    static webrtc::RTCErrorOr<VTEncoderSession> create(int32_t width, int32_t height,
                                                       CMVideoCodecType codecType,
                                                       bool hardwareAccelerated,
                                                       uint32_t qpMax = 100U,
                                                       VTEncoderSessionCallback* CM_NULLABLE callback = nullptr,
                                                       const std::shared_ptr<Bricks::Logger>& logger = {},
                                                       const std::shared_ptr<CFMemoryPool>& compressedDataAllocator = {});
private:
    VTEncoderSession(const std::shared_ptr<CFMemoryPool>& compressedDataAllocator,
                     std::unique_ptr<EncodePipeline> pipeline,
                     VTCompressionSessionRef CM_NONNULL sessionRef,
                     bool hardwareAccelerated,
                     int32_t width, int32_t height);
private:
    static CFMutableDictionaryRefAutoRelease encoderSpecification(bool hardwareAccelerated, int32_t width, int32_t height);
    static CFDictionaryRefAutoRelease sourceImageAttributes(int32_t width, int32_t height);
    static CFArrayRefAutoRelease createPixelFormats();
private:
    std::shared_ptr<CFMemoryPool> _compressedDataAllocator;
    std::unique_ptr<EncodePipeline> _pipeline;
    int32_t _width = 0;
    int32_t _height = 0;
};

} // namespace LiveKitCpp
