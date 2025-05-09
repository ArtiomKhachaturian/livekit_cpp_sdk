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
#include "MFNV12VideoPipeline.h"

namespace LiveKitCpp 
{

class MFVideoEncoderPipeline : public MFNV12VideoPipeline
{
public:
    // for real HWA choose MFT_ENUM_FLAG_HARDWARE instead of MFT_ENUM_FLAG_SYNCMFT
    static CompletionStatusOr<MFVideoEncoderPipeline> create(webrtc::VideoCodecType codecType,
                                                             UINT32 width,
                                                             UINT32 height,
                                                             UINT32 frameRate,
                                                             UINT32 avgBitsPerSecond,
                                                             UINT32 desiredFlags = MFT_ENUM_FLAG_SYNCMFT);
    static CompletionStatusOr<MFVideoEncoderPipeline>
        create(webrtc::VideoCodecType codecType, const GUID& encoder, UINT32 width,
               UINT32 height, UINT32 frameRate, UINT32 avgBitsPerSecond);
    MFVideoEncoderPipeline() = default;
    MFVideoEncoderPipeline(const MFVideoEncoderPipeline&) = default;
    MFVideoEncoderPipeline(MFVideoEncoderPipeline&&) = default;
    MFVideoEncoderPipeline& operator = (MFVideoEncoderPipeline&&) = default;
    MFVideoEncoderPipeline& operator = (const MFVideoEncoderPipeline&) = default;
    CComPtr<IMFMediaBuffer> createMediaBuffer(const webrtc::VideoFrame& frame) const;
    // https://learn.microsoft.com/en-us/windows/win32/medfound/mf-mt-mpeg-sequence-header-attribute
    CompletionStatusOr<std::vector<BYTE>> mpegSequenceHeader() const;
    CompletionStatus setSampleTimeMetrics(const CComPtr<IMFSample>& sample, const webrtc::VideoFrame& from);
    CompletionStatus setCompressedAvgBitrate(UINT32 bitsPerSecond);
    CompletionStatus setForceKeyFrame(bool force);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideotemporallayercount
    CompletionStatus setTemporalLayerCount(UINT32 count);
    CompletionStatus setH264Profile(eAVEncH264VProfile profile);
    CompletionStatus setH264Level(eAVEncH264VLevel level);
    CompletionStatus setH264CABACEnable(bool enable);
    // sets the sequence parameter set (SPS) identifier in the
    // SPS network abstraction layer (NAL) unit of the H.264 bit stream,
    // the valid range is 0 31, as specified in the H.264/AVC specification
    CompletionStatus setH264SPSID(UINT32 value);
    // sets the Picture Parameter Set (PPS) identifier
    CompletionStatus setH264PPSID(UINT32 value);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideoencodeqp
    CompletionStatus setVideoEncodeQP(UINT32 qp);
    CompletionStatus setCommonRateControlMode(eAVEncCommonRateControlMode mode);
    // 0 - min quality, 100 - max
    CompletionStatus setCommonQuality(UINT32 commonQuality);
    // 0 - lower quality, faster encoding, 100 - higher quality, slower encoding
    CompletionStatus setCommonQualityVsSpeed(UINT32 quality);
    //CODECAPI_AVEncCommonMinBitRate
    CompletionStatus setCommonMinBitRate(UINT32 bitsPerSecond);
    CompletionStatus setCommonMaxBitRate(UINT32 bitsPerSecond);
    CompletionStatus setCommonMeanBitRate(UINT32 bitsPerSecond);
    CompletionStatus setAdaptiveMode(bool adaptChangesOfResolution, bool adaptChangesOfFramerate);
    CompletionStatus setMPVGOPSize(UINT32 size);
    CompletionStatus setVideoContentType(eAVEncVideoContentType type);
    CompletionStatus setVideoContentType(bool camera);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideoltrbuffercontrol
    CompletionStatus setVideoLTRBufferControl(UINT32 control);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideomarkltrframe
    CompletionStatus setVideoMarkLTRFrame(bool set);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideouseltrframe
    CompletionStatus setVideoUseLTRFrame(UINT32 use);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideomaxnumrefframe
    CompletionStatus setVideoMaxNumRefFrame(UINT32 num);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideominqp
    CompletionStatus setVideoMinQP(UINT32 qp);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideomaxqp
    CompletionStatus setVideoMaxQP(UINT32 qp);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideoselectlayer
    CompletionStatus setVideoSelectLayer(UINT32 layer);
    // https://learn.microsoft.com/en-us/windows/win32/medfound/codecapi-avencvideotemporallayercount
    CompletionStatus setVideoTemporalLayerCount(UINT32 count);
protected:
    MFVideoEncoderPipeline(MFPipeline impl, webrtc::VideoCodecType codecType);
private:
    static CompletionStatusOr<MFVideoEncoderPipeline> 
        init(MFPipeline impl, webrtc::VideoCodecType codecType,
             UINT32 width, UINT32 height, UINT32 frameRate, UINT32 avgBitsPerSecond);
};

} // namespace LiveKitCpp