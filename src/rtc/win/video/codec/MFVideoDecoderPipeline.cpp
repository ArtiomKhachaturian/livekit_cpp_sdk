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
#include "MFVideoDecoderPipeline.h"
#include "MFCommon.h"
#include "MFVideoBuffer.h"
#include "Utils.h"
#include <Mferror.h>
#include <mfapi.h>
#include <rtc_base/ref_counted_object.h>

namespace LiveKitCpp 
{

MFVideoDecoderPipeline::MFVideoDecoderPipeline(MFPipeline impl,
                                               webrtc::VideoCodecType codecType,
                                               bool dxvaAccelerated)
    : MFNV12VideoPipeline(std::move(impl), codecType)
    , _dxvaAccelerated(dxvaAccelerated)
{
}

MFVideoDecoderPipeline::~MFVideoDecoderPipeline()
{
}

CompletionStatusOr<MFVideoDecoderPipeline> MFVideoDecoderPipeline::create(webrtc::VideoCodecType codecType,
                                                                          UINT32 width, UINT32 height,
                                                                          UINT32 desiredFlags)
{
    auto impl = createImpl(codecType, false, desiredFlags);
    if (!impl) {
        return impl.moveStatus();
    }
    return init(impl.moveValue(), codecType, width, height);
}

CompletionStatusOr<MFVideoDecoderPipeline> MFVideoDecoderPipeline::
    create(webrtc::VideoCodecType codecType, const GUID& decoder, UINT32 width, UINT32 height)
{
    auto impl = MFPipeline::create(false, decoder);
    if (!impl) {
        return impl.moveStatus();
    }
    return init(impl.moveValue(), codecType, width, height);
}

bool MFVideoDecoderPipeline::hardwareAccellerated() const noexcept
{
    return MFNV12VideoPipeline::hardwareAccellerated() || _dxvaAccelerated;
}

CompletionStatusOrScopedRefPtr<webrtc::VideoFrameBuffer> MFVideoDecoderPipeline::
    createBuffer(CComPtr<IMFMediaBuffer> inputBuffer, UINT32 width, UINT32 height) const
{
    if (!inputBuffer) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    MFMediaBufferLocker locker(std::move(inputBuffer), false);
    if (locker) {
        if (0U == width && 0U == height) {
            auto fs = uncompressedFrameSize();
            if (!fs) {
                return fs.moveStatus();
            }
            width = fs->first;
            height = fs->second;
        }
        auto buffer = MFMediaBuffer::create(width, height,
                                           VideoFrameType::NV12,
                                           std::move(locker),
                                           webrtc::VideoRotation::kVideoRotation_0,
                                           framesPool());
        if (buffer) {
            return buffer;
        }
        return COMPLETION_STATUS(MF_E_BUFFERTOOSMALL);
    }
    return locker.status();
}

CompletionStatus MFVideoDecoderPipeline::setMaxCodedWidth(UINT32 maxCodedWidth)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVDecVideoMaxCodedWidth, maxCodedWidth));
}

CompletionStatus MFVideoDecoderPipeline::setMaxCodedHeight(UINT32 maxCodedHeight)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVDecVideoMaxCodedHeight, maxCodedHeight));
}

CompletionStatus MFVideoDecoderPipeline::setSoftwareDynamicFormatChange(bool set)
{
    return COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVDecSoftwareDynamicFormatChange, set ? TRUE : FALSE));
}

CompletionStatus MFVideoDecoderPipeline::setSampleTimeMetrics(const CComPtr<IMFSample>& sample,
                                                              const webrtc::EncodedImage& from)
{
    return inputFramesTimeline().setSampleTimeMetrics(sample, from);
}

CompletionStatusOr<std::pair<UINT32, UINT32>> MFVideoDecoderPipeline::uncompressedFrameSize() const
{
    auto mt = uncompressedMediaType();
    if (mt) {
        auto fs = frameSize(mt.value());
        if (fs) {
            return fs.moveValue();
        }
        return fs.moveStatus();
    }
    return mt.moveStatus();
}

CompletionStatusOr<MFVideoArea> MFVideoDecoderPipeline::minimumDisplayAperture() const
{
    auto mt = uncompressedMediaType();
    if (mt) {
        // Query the visible area of the frame, which is the data that must be
        // returned to the caller. Sometimes this attribute is not present, and in
        // general this means that the frame size is not padded.
        MFVideoArea videoArea;
        auto status = COMPLETION_STATUS(mt.value()->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,
                                                            (UINT8*)&videoArea,
                                                            sizeof(videoArea),
                                                            NULL));
        if (status) {
            return videoArea;
        }
        return status;
    }
    return mt.moveStatus();
}

CompletionStatusOr<MFVideoDecoderPipeline> MFVideoDecoderPipeline::
    init(MFPipeline impl, webrtc::VideoCodecType codecType, UINT32 width, UINT32 height)
{
    if (!impl) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    bool dxvaAccelerated = false;
    if (webrtc::VideoCodecType::kVideoCodecH264 == codecType) {
        dxvaAccelerated = SUCCEEDED(impl.attributes()->SetUINT32(CODECAPI_AVDecVideoAcceleration_H264, TRUE));
    }
    auto compressedType = createCompressedMediaType(codecType);
    if (!compressedType) {
        return compressedType.moveStatus();
    }
    auto hr = setVideoInterlaceMode(compressedType.value(), MFVideoInterlace_MixedInterlaceOrProgressive);
    if (!hr) {
        return hr;
    }
    if (width > 0U && height > 0U) {
        hr = setFrameSize(compressedType.value(), width, height);
        if (!hr) {
            return hr;
        }
    }
    hr = setAllSamplesIndependent(compressedType.value(), false);
    if (!hr) {
        return hr;
    }
    hr = impl.setCompressedMediaType(compressedType.value());
    if (!hr) {
        return hr;
    }
    auto status = impl.compressedStatus();
    if (!status) {
        return status.moveStatus();
    }
    if (MFT_INPUT_STATUS_ACCEPT_DATA != status.value()) {
        return COMPLETION_STATUS(MF_E_TRANSFORM_NOT_POSSIBLE_FOR_CURRENT_INPUT_MEDIATYPE);
    }
    return MFVideoDecoderPipeline(std::move(impl), codecType, dxvaAccelerated);
}

} // namespace LiveKitCpp