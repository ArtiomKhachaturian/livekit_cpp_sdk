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
#include "MFNV12VideoPipeline.h"
#include "MFCommon.h"
#include "RtcUtils.h"
#include "VideoFrameBufferPoolSource.h"
#include <mfapi.h>
#include <processthreadsapi.h>
#include <cassert>

namespace LiveKitCpp 
{

MFNV12VideoPipeline::MFNV12VideoPipeline(MFPipeline impl, webrtc::VideoCodecType codecType)
    : _impl(std::move(impl))
    , _codecType(codecType)
    , _framesPool(VideoFrameBufferPoolSource::create())
{
    assert(_impl);
    _inputFramesTimeline.setSetupDuration(_impl.encoder());
}

bool MFNV12VideoPipeline::directXAccelerationSupported() const
{
    return TRUE == ::MFGetAttributeUINT32(_impl.attributes(), MF_SA_D3D_AWARE, FALSE);
}

bool MFNV12VideoPipeline::direct3D11Supported() const
{
    return TRUE == ::MFGetAttributeUINT32(_impl.attributes(), MF_SA_D3D11_AWARE, FALSE);
}

CompletionStatus MFNV12VideoPipeline::setCompressedFramerate(UINT32 frameRate)
{
    auto mt = compressedMediaType();
    if (mt) {
        return setFramerate(mt.value(), frameRate);
    }
    return mt.moveStatus();
}

CompletionStatus MFNV12VideoPipeline::setCompressedFrameSize(UINT32 width, UINT32 height)
{
    bool allowToChange = true;
    if (!encoder() && webrtc::VideoCodecType::kVideoCodecH264 == codecType()) {
        // The WMF H.264 decoder is documented to have a minimum resolution
        // 48x48 pixels for resolution, but we won't enable hw decoding for the
        // resolution < 132 pixels. It's assumed the software decoder doesn't
        // have this limitation, but it still might have maximum resolution
        // limitation.
        // https://msdn.microsoft.com/en-us/library/windows/desktop/dd797815(v=vs.85).aspx
        allowToChange = width >= 48U && height >= 48U;
        if (allowToChange) {
            allowToChange = width * height <= 4096 * 2304; // 4K is maximum
        }
    }
    if (allowToChange) {
        auto mt = compressedMediaType();
        if (mt) {
            return setFrameSize(mt.value(), width, height);
        }
        return mt.moveStatus();
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MFNV12VideoPipeline::setUncompressedFramerate(UINT32 frameRate)
{
    auto mt = uncompressedMediaType();
    if (mt) {
        return setFramerate(mt.value(), frameRate);
    }
    return mt.moveStatus();
}

CompletionStatus MFNV12VideoPipeline::setUncompressedFrameSize(UINT32 width, UINT32 height)
{
    auto mt = uncompressedMediaType();
    if (mt) {
        return setFrameSize(mt.value(), width, height);
    }
    return mt.moveStatus();
}

CompletionStatus MFNV12VideoPipeline::beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState)
{
    return _impl.beginGetEvent(callback, punkState);
}

CompletionStatus MFNV12VideoPipeline::endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent)
{
    return _impl.endGetEvent(result, outEvent);
}

CompletionStatusOrComPtr<IMFMediaEvent> MFNV12VideoPipeline::asyncEvent(DWORD flags) const
{
    return _impl.asyncEvent(flags);
}

CompletionStatus MFNV12VideoPipeline::setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return _impl.setCompressedMediaType(mediaType);
}

CompletionStatus MFNV12VideoPipeline::setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return _impl.setUncompressedMediaType(mediaType);
}

CompletionStatus MFNV12VideoPipeline::selectUncompressedMediaType()
{
    return _impl.selectMediaType(encoder(), uncompressedType());
}

CompletionStatus MFNV12VideoPipeline::setLowLatencyMode(bool set)
{
    return _impl.setLowLatencyMode(set);
}

CompletionStatus MFNV12VideoPipeline::setRealtimeContent(bool set)
{
    return _impl.setRealtimeContent(set);
}

CompletionStatus MFNV12VideoPipeline::setNumWorkerThreads(UINT32 numThreads)
{
    const auto& attribute = encoder() ? CODECAPI_AVEncNumWorkerThreads : CODECAPI_AVDecNumWorkerThreads;
    return COMPLETION_STATUS(setUINT32Attr(attribute, numThreads));
}

CompletionStatus MFNV12VideoPipeline::stop()
{
    auto hr = _impl.stop();
    if (hr) {
        _inputFramesTimeline.reset();
    }
    return hr;
}

CompletionStatusOr<DWORD> MFNV12VideoPipeline::uncompressedStatus() const
{
    auto hr = _impl.uncompressedStatus();
    if (!encoder()) {
        switch (codecType()) {
            case webrtc::VideoCodecType::kVideoCodecVP9:
                if (E_NOTIMPL == hr.code()) {
                    return MFT_OUTPUT_STATUS_SAMPLE_READY;
                }
                break;
            case webrtc::VideoCodecType::kVideoCodecH264:
                if (hr) {
                    // workaround [MF H264 bug: Output status is never set, even when ready]
                    // = > For now, always mark "ready" (results in extra buffer alloc / dealloc).
                    // don't MFT trust output status for now.
                    return MFT_OUTPUT_STATUS_SAMPLE_READY;
                }
                break;
            default:
                break;
        }
    }
    return hr;
}

CompletionStatusOr<INT32> MFNV12VideoPipeline::defaultStride() const
{
    return _impl.defaultStride();
}

CompletionStatus MFNV12VideoPipeline::processInput(const CComPtr<IMFSample>& sample, DWORD flags)
{
    return _impl.processInput(sample, flags);
}

CompletionStatusOrComPtr<IMFSample> MFNV12VideoPipeline::createSampleWitMemoryBuffer(bool input) const
{
    return _impl.createSampleWitMemoryBuffer(input);
}

CompletionStatusOrComPtr<IMFSample> MFNV12VideoPipeline::processOutput(const CComPtr<IMFSample>& sample,
                                                                       const CComPtr<IMFCollection>& events)
{
    return _impl.processOutput(sample, events);
}

CompletionStatus MFNV12VideoPipeline::processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    return _impl.processMessage(message, param);
}

const GUID& MFNV12VideoPipeline::compressedFormat(webrtc::VideoCodecType codecType)
{
    switch (codecType) {
        case webrtc::VideoCodecType::kVideoCodecVP8:
            return MFVideoFormat_VP80;
        case webrtc::VideoCodecType::kVideoCodecVP9:
            return MFVideoFormat_VP90;
        case webrtc::VideoCodecType::kVideoCodecH264:
            return MFVideoFormat_H264;
        case webrtc::VideoCodecType::kVideoCodecAV1:
            return MFVideoFormat_AV1;
        default:
            break;
    }
    return GUID_NULL;
}

const GUID& MFNV12VideoPipeline::uncompressedType()
{
    return MFVideoFormat_NV12;
}

CompletionStatusOrComPtr<IMFMediaType> MFNV12VideoPipeline::createUncompressedMediaType()
{
    return createMediaType(uncompressedType());
}

CompletionStatusOrComPtr<IMFMediaType> MFNV12VideoPipeline::createCompressedMediaType(webrtc::VideoCodecType codecType)
{
    return createMediaType(compressedFormat(codecType));
}

CompletionStatusOrComPtr<IMFMediaType> MFNV12VideoPipeline::createMediaType(const GUID& subType)
{
    auto type = LiveKitCpp::createMediaType(true, subType);
    if (type) {
        auto hr = setPixelAspectRatio1x1(type.value());
        if (hr) {
            return type;
        }
        return hr;
    }
    return type.moveStatus();
}

CompletionStatus MFNV12VideoPipeline::setAvgBitrate(const CComPtr<IMFMediaType>& mediaType, 
                                                    UINT32 bitsPerSecond)
{
    if (mediaType) {
        return COMPLETION_STATUS(mediaType->SetUINT32(MF_MT_AVG_BITRATE, bitsPerSecond));
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MFNV12VideoPipeline::setFrameParameters(const CComPtr<IMFMediaType>& mediaType,
                                                       UINT32 width, UINT32 height,
                                                       UINT32 frameRate, MFVideoInterlaceMode im,
                                                       bool allSamplesIndependent)
{
    auto hr = setFrameSize(mediaType, width, height);
    if (hr) {
        hr = setFramerate(mediaType, frameRate);
        if (hr) {
            hr = setVideoInterlaceMode(mediaType, im);
            if (hr) {
                hr = setAllSamplesIndependent(mediaType, allSamplesIndependent);
            }
        }
    }
    return hr;
}

CompletionStatus MFNV12VideoPipeline::setVideoInterlaceMode(const CComPtr<IMFMediaType>& mediaType,
                                                            MFVideoInterlaceMode im)
{
    if (mediaType) {
        return COMPLETION_STATUS(mediaType->SetUINT32(MF_MT_INTERLACE_MODE, im));
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

HRESULT MFNV12VideoPipeline::setUINT32Attr(const GUID& attribute, UINT32 value)
{
    return _impl.setUINT32Attr(attribute, value);
}

VideoFrameBufferPool MFNV12VideoPipeline::framesPool() const
{
    return VideoFrameBufferPool{_framesPool};
}

CompletionStatusOr<MFPipeline> MFNV12VideoPipeline::createImpl(webrtc::VideoCodecType codecType,
                                                               bool encoder,
                                                               bool sync,
                                                               bool hardwareAccellerated,
                                                               MFTransformConfigurator* configurator)
{
    const auto& codecFormat = compressedFormat(codecType);
    if (GUID_NULL != codecFormat) {
        // some HW encoders use DXGI API and crash when locked down
        if (encoder && hardwareAccellerated && isWin32LockedDown()) {
            hardwareAccellerated = false;
        }
        return MFPipeline::create(true, encoder, sync, hardwareAccellerated, false,
                                  codecFormat, uncompressedType(), configurator);
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

bool MFNV12VideoPipeline::isWin32LockedDown()
{
    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY polInfo = {0};
    if (::GetProcessMitigationPolicy(::GetCurrentProcess(), ProcessSystemCallDisablePolicy,
                                     &polInfo, sizeof(polInfo))) {
        return 1UL == polInfo.DisallowWin32kSystemCalls;
    }
    return false;
}

} // namespace LiveKitCpp