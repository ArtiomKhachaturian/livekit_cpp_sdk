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
#include "MFVideoEncoderPipeline.h"
#ifdef USE_PLATFORM_ENCODERS
#include "MFCommon.h"
#include "MFEncoderInputMediaBuffer.h"
#include "Utils.h"
#include <Mferror.h>
#include <mfapi.h>

namespace LiveKitCpp 
{

MFVideoEncoderPipeline::MFVideoEncoderPipeline(MFPipeline impl,
                                               webrtc::VideoCodecType codecType)
    : MFNV12VideoPipeline(std::move(impl), codecType)
{
}

CompletionStatusOr<MFVideoEncoderPipeline> MFVideoEncoderPipeline::
    create(webrtc::VideoCodecType codecType, UINT32 width, UINT32 height,
           UINT32 frameRate, UINT32 avgBitsPerSecond, UINT32 desiredFlags)
{
    auto impl = createImpl(codecType, true, desiredFlags);
    if (!impl) {
        return impl.moveStatus();
    }
    return init(impl.moveValue(), codecType, width, height, frameRate, avgBitsPerSecond);
}

CompletionStatusOr<MFVideoEncoderPipeline> MFVideoEncoderPipeline::
    create(webrtc::VideoCodecType codecType, const GUID& encoder, UINT32 width,
           UINT32 height, UINT32 frameRate, UINT32 avgBitsPerSecond)
{
    auto impl = MFPipeline::create(true, encoder);
    if (!impl) {
        return impl.moveStatus();
    }
    return init(impl.moveValue(), codecType, width, height, frameRate, avgBitsPerSecond);
}

CComPtr<IMFMediaBuffer> MFVideoEncoderPipeline::createMediaBuffer(const webrtc::VideoFrame& frame) const
{
    return MFEncoderInputMediaBuffer::create(frame, framesPool());
}

CompletionStatusOr<std::vector<BYTE>> MFVideoEncoderPipeline::mpegSequenceHeader() const
{
    auto compressed = compressedMediaType();
    if (compressed) {
        UINT32 headerSize = 0U;
        auto hr = COMPLETION_STATUS(compressed.value()->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER,
                                                                    &headerSize));
        if (MF_E_ATTRIBUTENOTFOUND == hr.code()) {
            return std::vector<BYTE>{};
        }
        if (hr) {
            std::vector<BYTE> headerData;
            if (headerSize) {
                headerData.resize(headerSize);
                hr = COMPLETION_STATUS(compressed.value()->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER,
                                                                   headerData.data(),
                                                                   headerSize, NULL));
            }
            if (hr) {
                return headerData;
            }
        }
        return hr;
    }
    return compressed.moveStatus();
}

CompletionStatus MFVideoEncoderPipeline::setSampleTimeMetrics(const CComPtr<IMFSample>& sample,
                                                            const webrtc::VideoFrame& from)
{
    return inputFramesTimeline().setSampleTimeMetrics(sample, from);
}

CompletionStatus MFVideoEncoderPipeline::setCompressedAvgBitrate(UINT32 bitsPerSecond)
{
    auto compressed = compressedMediaType();
    if (compressed) {
        return setAvgBitrate(compressed.value(), bitsPerSecond);
    }
    return compressed.moveStatus();
}

CompletionStatus MFVideoEncoderPipeline::setForceKeyFrame(bool force)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoForceKeyFrame, force ? TRUE : FALSE));
}

CompletionStatus MFVideoEncoderPipeline::setTemporalLayerCount(UINT32 count)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoTemporalLayerCount, count));
}

CompletionStatus MFVideoEncoderPipeline::setH264Profile(eAVEncH264VProfile profile)
{
    if (webrtc::VideoCodecType::kVideoCodecH264 == codecType()) {
        auto compressed = compressedMediaType();
        if (compressed) {
            return COMPLETION_STATUS(compressed.value()->SetUINT32(MF_MT_MPEG2_PROFILE, profile));
        }
        return compressed.moveStatus();
    }
    return COMPLETION_STATUS(E_NOTIMPL);
}

CompletionStatus MFVideoEncoderPipeline::setH264Level(eAVEncH264VLevel level)
{
    if (webrtc::VideoCodecType::kVideoCodecH264 == codecType()) {
        auto compressed = compressedMediaType();
        if (compressed) {
            return COMPLETION_STATUS(compressed.value()->SetUINT32(MF_MT_MPEG2_LEVEL, level));
        }
        return compressed.moveStatus();
    }
    return COMPLETION_STATUS(E_NOTIMPL);
}

CompletionStatus MFVideoEncoderPipeline::setH264CABACEnable(bool enable)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncH264CABACEnable, enable ? TRUE : FALSE));
}

CompletionStatus MFVideoEncoderPipeline::setH264SPSID(UINT32 value)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncH264SPSID, value));
}

CompletionStatus MFVideoEncoderPipeline::setH264PPSID(UINT32 value)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncH264PPSID, value));
}

CompletionStatus MFVideoEncoderPipeline::setVideoEncodeQP(UINT32 qp)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoEncodeQP, qp));
}

CompletionStatus MFVideoEncoderPipeline::setCommonRateControlMode(eAVEncCommonRateControlMode mode)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonRateControlMode, mode));
}

CompletionStatus MFVideoEncoderPipeline::setCommonQuality(UINT32 commonQuality)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonQuality, commonQuality));
}

CompletionStatus MFVideoEncoderPipeline::setCommonQualityVsSpeed(UINT32 quality)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonQualityVsSpeed, quality));
}

CompletionStatus MFVideoEncoderPipeline::setCommonMinBitRate(UINT32 bitsPerSecond)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonMinBitRate, bitsPerSecond));
}

CompletionStatus MFVideoEncoderPipeline::setCommonMaxBitRate(UINT32 bitsPerSecond)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonMaxBitRate, bitsPerSecond));
}

CompletionStatus MFVideoEncoderPipeline::setCommonMeanBitRate(UINT32 bitsPerSecond)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncCommonMeanBitRate, bitsPerSecond));
}

CompletionStatus MFVideoEncoderPipeline::setAdaptiveMode(bool adaptChangesOfResolution, bool adaptChangesOfFramerate)
{
    UINT32 mode = eAVEncAdaptiveMode::eAVEncAdaptiveMode_None;
    if (adaptChangesOfResolution) {
        mode |= eAVEncAdaptiveMode::eAVEncAdaptiveMode_Resolution;
    }
    if (adaptChangesOfFramerate) {
        mode |= eAVEncAdaptiveMode::eAVEncAdaptiveMode_FrameRate;
    }
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncAdaptiveMode, mode));
}

CompletionStatus MFVideoEncoderPipeline::setMPVGOPSize(UINT32 size)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncMPVGOPSize, size));
}

CompletionStatus MFVideoEncoderPipeline::setVideoContentType(eAVEncVideoContentType type)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoContentType, type));
}

CompletionStatus MFVideoEncoderPipeline::setVideoContentType(bool camera)
{
    eAVEncVideoContentType type = eAVEncVideoContentType::eAVEncVideoContentType_Unknown;
    if (camera) {
        type = eAVEncVideoContentType::eAVEncVideoContentType_FixedCameraAngle;
    }
    return setVideoContentType(type);
}

CompletionStatus MFVideoEncoderPipeline::setVideoLTRBufferControl(UINT32 control)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoLTRBufferControl, control));
}

CompletionStatus MFVideoEncoderPipeline::setVideoMarkLTRFrame(bool set)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoMarkLTRFrame, set ? TRUE : FALSE));
}

CompletionStatus MFVideoEncoderPipeline::setVideoUseLTRFrame(UINT32 use)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoUseLTRFrame, use));
}

CompletionStatus MFVideoEncoderPipeline::setVideoMaxNumRefFrame(UINT32 num)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoLTRBufferControl, num));
}

CompletionStatus MFVideoEncoderPipeline::setVideoMinQP(UINT32 qp)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoMinQP, qp));
}

CompletionStatus MFVideoEncoderPipeline::setVideoMaxQP(UINT32 qp)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoMaxQP, qp));
}

CompletionStatus MFVideoEncoderPipeline::setVideoSelectLayer(UINT32 layer)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoSelectLayer, layer));
}

CompletionStatus MFVideoEncoderPipeline::setVideoTemporalLayerCount(UINT32 count)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVEncVideoTemporalLayerCount, count));
}

CompletionStatusOr<MFVideoEncoderPipeline> MFVideoEncoderPipeline::
    init(MFPipeline impl, webrtc::VideoCodecType codecType, UINT32 width, UINT32 height, 
         UINT32 frameRate, UINT32 avgBitsPerSecond)
{
    if (!impl) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    auto output = createCompressedMediaType(codecType);
    if (!output) {
        return output.moveStatus();
    }
    auto hr = setFrameParameters(output.value(), width, height, frameRate,
                                 MFVideoInterlace_Progressive, true);
    if (!hr) {
        return hr;
    }
    // [setAvgBitrate] is mandatory, otherwise E_FAIL on [setCompressedMediaType]
    hr = setAvgBitrate(output.value(), avgBitsPerSecond);
    if (!hr) {
        return hr;
    }
    // always set encoder output (compressed) type before input
    hr = impl.setCompressedMediaType(output.value());
    if (!hr) {
        return hr;
    }
    auto input = createUncompressedMediaType();
    if (!input) {
        return input.moveStatus();
    }
    hr = setFrameParameters(input.value(), width, height, frameRate, 
                            MFVideoInterlace_Progressive, true);
    if (!hr) {
        return hr;
    }
    hr = impl.setUncompressedMediaType(input.value());
    if (!hr) {
        return hr;
    }
    return MFVideoEncoderPipeline(std::move(impl), codecType);
}

} // namespace LiveKitCpp
#endif