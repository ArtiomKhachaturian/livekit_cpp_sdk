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
#include <rtc_base/logging.h>
#include "MFVideoDecoder.h"
#ifdef USE_PLATFORM_DECODERS
#include "MFCommon.h"
#include "MFDecoderInputMediaBuffer.h"
#include "MFInitializer.h"
#include "MFVideoBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "Utils.h"
#include "VideoUtils.h"
#include "VideoFrameBufferPoolSource.h"
#include <api/make_ref_counted.h>
#include <rtc_base/time_utils.h>
#include <codecapi.h>
#include <Mferror.h>
#include <mfapi.h>
#include <mfidl.h>

namespace LiveKitCpp 
{

class MFVideoDecoder::NV12MisalignedBuffer : public NV12VideoFrameBuffer
{
public:
    NV12MisalignedBuffer(MFMediaBufferLocker locker, int width, int height,
                         const BYTE* srcUV, int strideY, int strideUV,
                         VideoFrameBufferPool framesPool);
    // impl. of NV12VideoFrameBuffer
    int width() const final { return _width; }
    int height() const final { return _height; }
    int StrideY() const final { return _strideY; }
    int StrideUV() const final { return _strideUV; }
    const uint8_t* DataY() const final { return _locker.dataBuffer(); }
    const uint8_t* DataUV() const final { return _srcUV; }
private:
    const MFMediaBufferLocker _locker;
    const int _width;
    const int _height;
    const BYTE* const _srcUV;
    const int _strideY;
    const int _strideUV;
};

MFVideoDecoder::MFVideoDecoder(const webrtc::SdpVideoFormat& format)
    : VideoDecoder(format)
    , _framesPool(VideoFrameBufferPoolSource::create())
{
    _inputFramesTimeline.setSetupDuration(false);
}

MFVideoDecoder::~MFVideoDecoder()
{
}

bool MFVideoDecoder::hardwareAccelerated() const
{
    if (_decoder) {
        CComPtr<IMFAttributes> attributes;
        if (SUCCEEDED(_decoder->GetAttributes(&attributes))) {
            return videoAccelerated(type(), attributes);
        }
    }
    return false;
}

bool MFVideoDecoder::Configure(const Settings& settings)
{
    if (VideoDecoder::Configure(settings)) {
        auto status = COMPLETION_STATUS(initializeComForThisThread().status());
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        status = COMPLETION_STATUS(MFInitializer::initializeForThisThread().status());
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        const auto& format = compressedType(type());
        CComPtr<IMFTransform> decoder;
        status = COMPLETION_STATUS(::CoCreateInstance(predefinedCodecType(false, format), 
                                                      NULL, CLSCTX_INPROC_SERVER, IID_IUnknown,
                                                      (void**)&decoder));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        CComPtr<IMFAttributes> decoderAttrs;
        status = COMPLETION_STATUS(decoder->GetAttributes(&decoderAttrs));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        status = COMPLETION_STATUS(decoderAttrs->SetUINT32(CODECAPI_AVLowLatencyMode, TRUE));
        if (!status) {
            RTC_LOG(LS_WARNING) << status;
        }
        if (webrtc::VideoCodecType::kVideoCodecH264 == type()) {
            status = COMPLETION_STATUS(decoderAttrs->SetUINT32(CODECAPI_AVDecVideoAcceleration_H264, TRUE));
            if (!status) {
                RTC_LOG(LS_WARNING) << status;
            }
        }
        UINT32 width = 0U, height = 0U;
        if (settings.max_render_resolution().Valid()) {
            width = static_cast<UINT32>(settings.max_render_resolution().Width());
            height = static_cast<UINT32>(settings.max_render_resolution().Height());
        }
        if (width && height && !videoAccelerated(type(), decoderAttrs)) {
            const auto threads = maxDecodingThreads(width, height, settings.number_of_cores());
            status = COMPLETION_STATUS(decoderAttrs->SetUINT32(CODECAPI_AVDecNumWorkerThreads, TRUE));
            if (!status) {
                RTC_LOG(LS_WARNING) << status;
            }
        }
        const auto inputMedia = createInputMedia(format, width, height);
        if (!inputMedia) {
            RTC_LOG(LS_ERROR) << inputMedia.status();
            return false;
        }
        status = COMPLETION_STATUS(decoder->SetInputType(0U, inputMedia.value(), 0U));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        // Assert MF supports NV12 output
        bool suitableTypeFound = false;
        status = configureOutputMedia(decoder, MFVideoFormat_NV12, suitableTypeFound);
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        if (!suitableTypeFound) {
            RTC_LOG(LS_ERROR) << "failed to find a valid output NV12 media type for decoding";
            return false;
        }
        DWORD decoderStatus = 0U;
        status = COMPLETION_STATUS(decoder->GetInputStatus(0U, &decoderStatus));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        if (MFT_INPUT_STATUS_ACCEPT_DATA != decoderStatus) {
            RTC_LOG(LS_ERROR) << "video decoder MFT is not accepting data";
            return false;
        }
        status = COMPLETION_STATUS(decoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        status = COMPLETION_STATUS(decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        status = COMPLETION_STATUS(decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return false;
        }
        _decoder = std::move(decoder);
        if (_framesPool) {
            _framesPool->resize(static_cast<size_t>(bufferPoolSize()));
        }
        return true;
    }
    return false;
}

int32_t MFVideoDecoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs)
{
    if (!_decoder || !hasDecodeCompleteCallback()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (!inputImage.data() && inputImage.size() > 0U) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    // discard until keyframe
    if (_requestKeyFrame) {
        if (webrtc::VideoFrameType::kVideoFrameKey != inputImage.FrameType()) {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        _requestKeyFrame = false;
    }
    // enqueue the new frame with Media Foundation
    auto status = enqueueFrame(inputImage, missingFrames);
    if (MF_E_NOTACCEPTING == status.code()) {
        // For robustness (shouldn't happen). Flush any old MF data blocking the
        // new frames.
        status = COMPLETION_STATUS(_decoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL));
        if (webrtc::VideoFrameType::kVideoFrameKey == inputImage.FrameType()) {
            status = enqueueFrame(inputImage, missingFrames);
        }
        else {
            _requestKeyFrame = true;
            if (!status) {
                RTC_LOG(LS_ERROR) << status;
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            return WEBRTC_VIDEO_CODEC_OK_REQUEST_KEYFRAME;
        }
    }
    if (!status) {
        RTC_LOG(LS_ERROR) << status;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    // Flush any decoded samples resulting from new frame, invoking callback
    status = flushFrames(inputImage);
    if (MF_E_TRANSFORM_STREAM_CHANGE == status.code()) {
        // Output media type is no longer suitable. Reconfigure and retry.
        bool suitableTypeFound = false;
        status = configureOutputMedia(_decoder, MFVideoFormat_NV12, suitableTypeFound);
        if (!status || !suitableTypeFound) {
            if (!status) {
                RTC_LOG(LS_ERROR) << status;
            }
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        status = flushFrames(inputImage);
    }
    if (status || MF_E_TRANSFORM_NEED_MORE_INPUT == status.code()) {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    return WEBRTC_VIDEO_CODEC_ERROR;
}

webrtc::VideoDecoder::DecoderInfo MFVideoDecoder::GetDecoderInfo() const
{
    auto info = VideoDecoder::GetDecoderInfo();
    if (_decoder) {
        auto name = transformFriendlyName(_decoder);
        if (name) {
            info.implementation_name = name.moveValue();
        }
    }
    return info;
}

CompletionStatus MFVideoDecoder::destroySession()
{
    if (_decoder) {
        // follow shutdown procedure gracefully, on fail, continue anyway
        auto status = COMPLETION_STATUS(_decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0));
        if (!status) {
            RTC_LOG(LS_WARNING) << status;
        }
        status = COMPLETION_STATUS(_decoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0));
        if (!status) {
            RTC_LOG(LS_WARNING) << status;
        }
        status = COMPLETION_STATUS(_decoder->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0));
        if (!status) {
            RTC_LOG(LS_WARNING) << status;
        }
        _decoder.Release();
        _requestKeyFrame = false;
        if (_framesPool) {
            _framesPool->release();
        }
    }
    _inputFramesTimeline.reset();
    return VideoDecoder::destroySession();
}

CompletionStatusOrComPtr<IMFMediaType> MFVideoDecoder::createInputMedia(const GUID& format,
                                                                        UINT32 width, UINT32 height,
                                                                        UINT32 fps)
{
    // output media type
    auto mediaType = createMediaType(true, format);
    if (!mediaType) {
        return mediaType.moveStatus();
    }
    auto status = setPixelAspectRatio1x1(mediaType.value());
    if (!status) {
        return status;
    }
    status = setInterlaceMode(mediaType.value(), MFVideoInterlace_MixedInterlaceOrProgressive);
    if (!status) {
        return status;
    }
    if (width && height) {
        status = setFrameSize(mediaType.value(), width, height);
        if (!status) {
            return status;
        }
    }
    if (fps) {
        status = setFramerate(mediaType.value(), fps);
        if (!status) {
            return status;
        }
    }
    return mediaType;
}

CompletionStatus MFVideoDecoder::configureOutputMedia(const CComPtr<IMFTransform>& decoder,
                                                      const GUID& format, bool& typeFound)
{
    if (!decoder) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    DWORD typeIndex = 0U;
    while (true) {
        CComPtr<IMFMediaType> outputMedia;
        auto status = COMPLETION_STATUS(decoder->GetOutputAvailableType(0, typeIndex++, &outputMedia));
        if (MF_E_NO_MORE_TYPES == status.code()) {
            return {};
        }
        if (!status) {
            return status;
        }
        GUID curType;
        status = COMPLETION_STATUS(outputMedia->GetGUID(MF_MT_SUBTYPE, &curType));
        if (!status) {
            return status;
        }
        if (format == curType) {
            status = COMPLETION_STATUS(decoder->SetOutputType(0U, outputMedia, 0U));
            if (!status) {
                return status;
            }
            outputMedia->SetUINT32(MF_MT_REALTIME_CONTENT, TRUE); // ignore errors
            typeFound = true;
            break;
        }
    }
    return {};
}

bool MFVideoDecoder::videoAccelerated(webrtc::VideoCodecType type, const CComPtr<IMFAttributes>& attributes)
{
    bool hwa = false;
    if (attributes) {
        const auto flags = ::MFGetAttributeUINT32(attributes, MF_TRANSFORM_FLAGS_Attribute, 0U);
        hwa = testFlag<MFT_ENUM_FLAG_HARDWARE>(flags);
        if (!hwa && webrtc::VideoCodecType::kVideoCodecH264 == type) {
            UINT32 h264Accel = FALSE;
            if (SUCCEEDED(attributes->GetUINT32(CODECAPI_AVDecVideoAcceleration_H264, &h264Accel))) {
                hwa = TRUE == h264Accel;
            }
        }
    }
    return hwa;
}

CompletionStatus MFVideoDecoder::enqueueFrame(const webrtc::EncodedImage& inputImage, bool missingFrames)
{
    if (!_decoder) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    const CComPtr<IMFMediaBuffer> mediaBuffer{MFDecoderInputMediaBuffer::create(inputImage.GetEncodedData())};
    if (!mediaBuffer) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    CComPtr<IMFSample> sample;
    auto status = COMPLETION_STATUS(::MFCreateSample(&sample));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(sample->AddBuffer(mediaBuffer));
    if (!status) {
        return status;
    }
    status = _inputFramesTimeline.setTimeMetrics(inputImage, sample);
    if (!status) {
        return status;
    }
    CComPtr<IMFAttributes> attributes;
    status = COMPLETION_STATUS(sample->QueryInterface(&attributes));
    if (!status) {
        RTC_LOG(LS_WARNING) << status;
    }
    if (attributes) {
        if(webrtc::VideoFrameType::kVideoFrameKey == inputImage.FrameType()) {
            status = COMPLETION_STATUS(attributes->SetUINT32(MFSampleExtension_CleanPoint, TRUE));
            if (!status) {
                RTC_LOG(LS_WARNING) << status;
            }
            const auto realtime = webrtc::VideoContentType::UNSPECIFIED == inputImage.contentType();

        }
        if (missingFrames) {
            status = COMPLETION_STATUS(attributes->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
            if (!status) {
                RTC_LOG(LS_WARNING) << status;
            }
        }
    }
    return COMPLETION_STATUS(_decoder->ProcessInput(0U, sample, 0U));
}

CompletionStatus MFVideoDecoder::flushFrames(const webrtc::EncodedImage& inputImage)
{
    if (!_decoder) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    CompletionStatus hr;
    while (hr && hasDecodeCompleteCallback()) {
        auto status = outputStatus();
        if (!status) {
            hr = status.moveStatus();
            break;
        }
        if (MFT_OUTPUT_STATUS_SAMPLE_READY != status.value()) {
            break;
        }
        // Get needed size of our output buffer
        MFT_OUTPUT_STREAM_INFO streamInfo;
        hr = COMPLETION_STATUS(_decoder->GetOutputStreamInfo(0, &streamInfo));
        if (!hr) {
            break;
        }
        auto sample = createSampleWitMemoryBuffer(streamInfo.cbSize, streamInfo.cbAlignment);
        if (!sample) {
            hr = sample.moveStatus();
            break;
        }
        // Create output buffer description
        MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
        outputDataBuffer.dwStatus = 0;
        outputDataBuffer.dwStreamID = 0;
        outputDataBuffer.pEvents = NULL;
        outputDataBuffer.pSample = sample.value();
        // Invoke the Media Foundation decoder
        // Note: we don't use ON_SUCCEEDED here since ProcessOutput returns
        //       MF_E_TRANSFORM_NEED_MORE_INPUT often (too many log messages).
        DWORD outputStatus = 0U;
        hr = COMPLETION_STATUS(_decoder->ProcessOutput(0, 1, &outputDataBuffer, &outputStatus));
        if (!hr) {
            // can return MF_E_TRANSFORM_NEED_MORE_INPUT or
            // MF_E_TRANSFORM_STREAM_CHANGE (entirely acceptable)
            break;
        }
        hr = sendDecodedSample(inputImage, sample.value());
    }
    return hr;
}

CompletionStatus MFVideoDecoder::sendDecodedSample(const webrtc::EncodedImage& inputImage, 
                                                   const CComPtr<IMFSample>& sample)
{
    if (!sample) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    // copy raw output sample data to video frame buffer.
    CComPtr<IMFMediaBuffer> srcBuffer;
    auto status = COMPLETION_STATUS(sample->ConvertToContiguousBuffer(&srcBuffer));
    if (!status) {
        return status;
    }
    // Get the output media type
    CComPtr<IMFMediaType> outputType;
    status = COMPLETION_STATUS(_decoder->GetOutputCurrentType(0U, &outputType));
    if (!status) {
        return status;
    }
    // Query the actual buffer size to get the stride and vertical padding, so
    // the UV plane can be accessed at the right offset.
    UINT32 width = 0U, height = 0U, bufferWidth = 0U, bufferHeight = 0U;
    status = COMPLETION_STATUS(::MFGetAttributeSize(outputType, MF_MT_FRAME_SIZE, &bufferWidth, &bufferHeight));
    if (!status) {
        return status;
    }
    // Query the visible area of the frame, which is the data that must be
    // returned to the caller. Sometimes this attribute is not present, and in
    // general this means that the frame size is not padded.
    MFVideoArea videoArea;
    if (SUCCEEDED(outputType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, 
                                      (UINT8*)&videoArea, 
                                      sizeof(videoArea), NULL))) {
        width = videoArea.Area.cx;
        height = videoArea.Area.cy;
    }
    else {
        // Fallback to buffer size
        width = bufferWidth;
        height = bufferHeight;
    }
    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer;
    MFMediaBufferLocker locker(std::move(srcBuffer), false);
    if (!locker) {
        return locker.status();
    }
    if (width == bufferWidth && height == bufferHeight) {
        buffer = MFMediaRawBuffer::create(width, height,
                                          VideoFrameType::NV12,
                                          std::move(locker),
                                          webrtc::VideoRotation::kVideoRotation_0,
                                          VideoFrameBufferPool{_framesPool});
    }
    else {
        const int srcStrideY = bufferWidth;
        const int srcStrideUV = bufferWidth;
        const BYTE* srcUV = locker.dataBuffer() + (srcStrideY * bufferHeight);
        buffer = webrtc::make_ref_counted<NV12MisalignedBuffer>(std::move(locker),
                                                                width, height, srcUV, 
                                                                srcStrideY, srcStrideUV,
                                                                VideoFrameBufferPool{_framesPool});
    }
    auto frame = createVideoFrame(buffer, webrtc::VideoRotation::kVideoRotation_0,
                                  inputImage.NtpTimeMs() * rtc::kNumMicrosecsPerMillisec);
    if (!frame) {
        return COMPLETION_STATUS(MF_E_BUFFERTOOSMALL);
    }
    frame->set_rtp_timestamp(inputImage.RtpTimestamp());
    sendDecodedImage(frame.value(), std::nullopt, lastSliceQp(inputImage));
    return {};
}

/**
 * Workaround [MF H264 bug: Output status is never set, even when ready]
 *  => For now, always mark "ready" (results in extra buffer alloc/dealloc).
 */
CompletionStatusOr<DWORD> MFVideoDecoder::outputStatus() const
{
    if (!_decoder) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    DWORD outputStatus = 0U;
    auto status = COMPLETION_STATUS(_decoder->GetOutputStatus(&outputStatus));
    if (!status) {
        return status;
    }
    if (webrtc::VideoCodecType::kVideoCodecH264 == type()) {
        // Don't MFT trust output status for now.
        outputStatus = MFT_OUTPUT_STATUS_SAMPLE_READY;
    }
    return outputStatus;
}


MFVideoDecoder::NV12MisalignedBuffer::NV12MisalignedBuffer(MFMediaBufferLocker locker,
                                                           int width, int height,
                                                           const BYTE* srcUV, int strideY, int strideUV,
                                                           VideoFrameBufferPool framesPool)
    : NV12VideoFrameBuffer(std::move(framesPool))
    , _locker(std::move(locker))
    , _width(width)
    , _height(height)
    , _srcUV(srcUV)
    , _strideY(strideY)
    , _strideUV(strideUV)
{
}

} // namespace LiveKitCpp
#endif