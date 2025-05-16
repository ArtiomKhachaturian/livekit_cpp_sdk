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
#include "MFVideoEncoder.h"
#ifdef USE_PLATFORM_ENCODERS
#include "EncodedImageBuffer.h"
#include "MFCommon.h"
#include "MFEncoderInputMediaBuffer.h"
#include "MFInitializer.h"
#include "MFMediaBufferLocker.h"
#include "VideoFrameBufferPoolSource.h"
#include "Utils.h"
#include <Mfapi.h>
#include <Mferror.h>
#include <mfobjects.h>
#include <codecapi.h>
#include <rtc_base/time_utils.h>
#include <rtc_base/logging.h>

namespace LiveKitCpp 
{

class MFVideoEncoder::EncodedBuffer : public webrtc::EncodedImageBufferInterface
{
public:
    EncodedBuffer(MFMediaBufferLocker locker);
    // impl. of webrtc::EncodedImageBufferInterface
    const uint8_t* data() const final { return _locker.dataBuffer(); }
    uint8_t* data() final { return _locker.dataBuffer(); }
    size_t size() const final { return _locker.currentLen(); }
private:
    const MFMediaBufferLocker _locker;
};

MFVideoEncoder::MFVideoEncoder(const webrtc::SdpVideoFormat& format, webrtc::CodecSpecificInfo codecSpecificInfo)
    : VideoEncoder(format, std::move(codecSpecificInfo), false)
    , _framesPool(VideoFrameBufferPoolSource::create())
{
    _inputFramesTimeline.setSetupDuration(true);
}

int32_t MFVideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t r = VideoEncoder::InitEncode(codecSettings, encoderSettings);
    if (WEBRTC_VIDEO_CODEC_OK == r) {
        UINT32 targetBps = 0U;
        if (codecSettings->startBitrate) {
            targetBps = codecSettings->startBitrate * 1000;
        }
        else if (minBitrateBps()) {
            targetBps = minBitrateBps();
        }
        else {
            targetBps = std::min(maxBitrateBps(), codecSettings->width * codecSettings->height * 2U);
        }
        const auto status = initWriter(codecSettings->width, codecSettings->height, targetBps, maxFramerate(), qpMax());
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            r = WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    return r;
}

int32_t MFVideoEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes)
{
    if (!_sinkWriter || !hasEncodeCompleteCallback()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    const auto now = webrtc::TimeMillis();
    const UINT32 width = frame.width();
    const UINT32 height = frame.height();
    // Reset the encoder configuration if necessary.
    const auto resChanged = width != _width || height != _height;
    const auto shouldChangeRateNow = _rateChangeRequested && (now - _lastRateChangeTimeMs) > _minIntervalBetweenRateChangesMs;
    if (resChanged || shouldChangeRateNow) {
        const auto status = reconfigureWriter(width, height, currentBitrate(), currentFramerate());
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
        _rateChangeRequested = false;
    }
    if (frameTypes) {
        CompletionStatus status;
        for (auto frameType : *frameTypes) {
            if (webrtc::VideoFrameType::kVideoFrameKey == frameType) {
                Microsoft::WRL::ComPtr<IMFSinkWriterEncoderConfig> encoderConfig;
                status = COMPLETION_STATUS(_sinkWriter.As(&encoderConfig));
                if (!status) {
                    break;
                }
                CComPtr<IMFAttributes> encoderAttributes;
                status = COMPLETION_STATUS(::MFCreateAttributes(&encoderAttributes, 1));
                if (!status) {
                    break;
                }
                status = COMPLETION_STATUS(encoderAttributes->SetUINT32(CODECAPI_AVEncVideoForceKeyFrame, TRUE));
                if (!status) {
                    break;
                }
                status = COMPLETION_STATUS(encoderConfig->PlaceEncodingParameters(_streamIndex, encoderAttributes));
                break;
            }
        }
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
    }
    CComPtr<IMFSample> sample;
    // only encode the frame if the encoder pipeline is not full
    if (_sampleAttributeQueue.size() <= 2U) {
        sample = fromVideoFrame(frame);
    }
    if (!sample) {
        // drop the frame, send a tick to keep the encoder going
        _lastInputFrameDropped = true;
        _inputFramesTimeline.setTimeMetrics(frame);
        const auto status = COMPLETION_STATUS(_sinkWriter->SendStreamTick(_streamIndex, _inputFramesTimeline.lastTimestampHns()));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }
    auto status = COMPLETION_STATUS(_sinkWriter->WriteSample(_streamIndex, sample));
    if (!status) {
        RTC_LOG(LS_ERROR) << status;
        return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    if (0U == ++_framesCount % currentFramerate()) {
        // some threads online mention this is useful to do regularly
        status = COMPLETION_STATUS(_sinkWriter->NotifyEndOfSegment(_streamIndex));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

webrtc::VideoEncoder::EncoderInfo MFVideoEncoder::GetEncoderInfo() const
{
    auto encoderInfo = VideoEncoder::GetEncoderInfo();
    addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type::kNV12, encoderInfo);
    return encoderInfo;
}

CompletionStatus MFVideoEncoder::destroySession()
{
    auto status = releaseWriter();
    if (!status) {
        return status;
    }
    _inputFramesTimeline.reset();
    _lastInputFrameDropped = false;
    return VideoEncoder::destroySession();
}

CompletionStatus MFVideoEncoder::setEncoderBitrate(uint32_t bitrateBps)
{
    if (_sinkWriter) {
        const auto now = webrtc::TimeMillis();
        const auto timeToWaitBeforeRateChange = _minIntervalBetweenRateChangesMs - (now - _lastRateChangeTimeMs);
        if (timeToWaitBeforeRateChange > 0) {
            _rateChangeRequested = true;
            return {};
        }
        return reconfigureWriter(_width, _height, bitrateBps, currentFramerate());
    }
    return {};
}

CompletionStatus MFVideoEncoder::setEncoderFrameRate(uint32_t frameRate)
{
    if (_framesPool) {
        _framesPool->resize(frameRate);
    }
    if (_sinkWriter) {
        const auto now = webrtc::TimeMillis();
        const auto timeToWaitBeforeRateChange = _minIntervalBetweenRateChangesMs - (now - _lastRateChangeTimeMs);
        if (timeToWaitBeforeRateChange > 0) {
            _rateChangeRequested = true;
            return {};
        }
        return reconfigureWriter(_width, _height, currentBitrate(), frameRate);
    }
    return {};
}

CompletionStatusOr<webrtc::EncodedImage> MFVideoEncoder::createEncodedImage(bool keyFrame,
                                                                            IMFMediaBuffer* data,
                                                                            const std::optional<UINT32>& encodedQp)
{
    if (!data) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    MFMediaBufferLocker locker(data, false);
    if (!locker) {
        return locker.status();
    }
    webrtc::EncodedImage image;
    image.SetEncodedData(webrtc::make_ref_counted<EncodedBuffer>(std::move(locker)));
    if (encodedQp) {
        image.qp_ = encodedQp.value();
    }
    return image;
}

std::optional<UINT32> MFVideoEncoder::encoderQp(IMFSample* sample)
{
    if (sample) {
        // For HMFT that continuously reports valid QP, update encoder info so that
        // WebRTC will not use bandwidth quality scaler for resolution adaptation.
        UINT64 qp = 0xfffful;
        const auto hr = sample->GetUINT64(MFSampleExtension_VideoEncodeQP, &qp);
        if (SUCCEEDED(hr)) {
            // bits 0-15: default QP
            return static_cast<UINT32>(qp & 0xfffful);
        }
    }
    return std::nullopt;
}

CComPtr<IMFSample> MFVideoEncoder::fromVideoFrame(const webrtc::VideoFrame& frame)
{
    CComPtr<IMFMediaBuffer> buffer = MFEncoderInputMediaBuffer::create(frame, {_framesPool});
    if (buffer) {
        auto result = createSample(buffer);
        if (!result) {
            RTC_LOG(LS_ERROR) << result.status();
            return {};
        }
        auto sample = result.moveValue();
        auto status = _inputFramesTimeline.setTimeMetrics(frame, sample);
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return {};
        }
        if (_lastInputFrameDropped || dropNextFrame()) {
            CComPtr<IMFAttributes> sampleAttributes;
            status = COMPLETION_STATUS(sample.QueryInterface<IMFAttributes>(&sampleAttributes));
            if (!status) {
                RTC_LOG(LS_ERROR) << status;
                return {};
            }
            status = COMPLETION_STATUS(sampleAttributes->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
            if (!status) {
                RTC_LOG(LS_ERROR) << status;
                return {};
            }
            _lastInputFrameDropped = false;
        }
        _sampleAttributeQueue.push(_inputFramesTimeline.lastTimestampHns(), VideoFrameInfo(frame));
        return sample;
    }
    return {};
}

CompletionStatus MFVideoEncoder::initWriter(UINT32 width, UINT32 height, UINT32 targetBps,
                                            UINT32 framerate, UINT32 maxQP)
{
    auto status = COMPLETION_STATUS(initializeComForThisThread().status());
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(MFInitializer::initializeForThisThread().status());
    if (!status) {
        return status;
    }
    // output media type
    CComPtr<IMFMediaType> mediaTypeOut;
    status = COMPLETION_STATUS(::MFCreateMediaType(&mediaTypeOut));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, compressedType(type())));
    if (!status) {
        return status;
    }
    // Lumia 635 and Lumia 1520 Windows phones don't work well
    // with constrained baseline profile.
    // ON_SUCCEEDED(mediaTypeOut->SetUINT32(MF_MT_MPEG2_PROFILE,
    // eAVEncH264VProfile_ConstrainedBase));
    status = COMPLETION_STATUS(mediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, targetBps));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(MFSetAttributeSize(mediaTypeOut, MF_MT_FRAME_SIZE, width, height));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(MFSetAttributeRatio(mediaTypeOut, MF_MT_FRAME_RATE, framerate, 1));
    if (!status) {
        return status;
    }
    // input media type (nv12)
    CComPtr<IMFMediaType> mediaTypeIn;
    status = COMPLETION_STATUS(::MFCreateMediaType(&mediaTypeIn));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(mediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(::MFSetAttributeSize(mediaTypeIn, MF_MT_FRAME_SIZE, width, height));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(::MFSetAttributeRatio(mediaTypeIn, MF_MT_FRAME_RATE, framerate, 1));
    if (!status) {
        return status;
    }
    Microsoft::WRL::ComPtr<MFMediaSink> mediaSink;
    // Create the media sink
    status = COMPLETION_STATUS(Microsoft::WRL::MakeAndInitialize<LiveKitCpp::MFMediaSink>(&mediaSink));
    if (!status) {
        return status;
    }
    // SinkWriter creation attributes
    CComPtr<IMFAttributes> sinkWriterAttributes;
    status = COMPLETION_STATUS(::MFCreateAttributes(&sinkWriterAttributes, 1));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(sinkWriterAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(sinkWriterAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(sinkWriterAttributes->SetUINT32(MF_LOW_LATENCY, TRUE));
    if (!status) {
        return status;
    }
    Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter;
    // Create the sink writer
    status = COMPLETION_STATUS(::MFCreateSinkWriterFromMediaSink(mediaSink.Get(), sinkWriterAttributes, &sinkWriter));
    if (!status) {
        return status;
    }
    DWORD streamIndex = {};
    // Add the h264 output stream to the writer
    status = COMPLETION_STATUS(sinkWriter->AddStream(mediaTypeOut, &streamIndex));
    if (!status) {
        return status;
    }
    // SinkWriter encoder properties
    CComPtr<IMFAttributes> encodingAttributes;
    status = COMPLETION_STATUS(::MFCreateAttributes(&encodingAttributes, 1));
    if (!status) {
        return status;
    }
    if (maxQP) {
        status = COMPLETION_STATUS(encodingAttributes->SetUINT32(CODECAPI_AVEncVideoMaxQP, maxQP));
        if (!status) {
            return status;
        }
    }
    status = COMPLETION_STATUS(sinkWriter->SetInputMediaType(streamIndex,  mediaTypeIn, encodingAttributes));
    if (!status) {
        return status;
    }
    // Register this as the callback for encoded samples.
    status = COMPLETION_STATUS(mediaSink->registerEncodingCallback(this));
    if (!status) {
        return status;
    }
    status = COMPLETION_STATUS(sinkWriter->BeginWriting());
    if (status) {
        _mediaSink = std::move(mediaSink);
        _sinkWriter = std::move(sinkWriter);
        _streamIndex = streamIndex;
        _width = width;
        _height = height;
        _lastRateChangeTimeMs = webrtc::TimeMillis();
    }
    return status;
}

CompletionStatus MFVideoEncoder::releaseWriter()
{
    if (_sinkWriter) {
        //_sinkWriter->Finalize();
        _sinkWriter.Reset();
        _framesCount = 0ULL;
        _sampleAttributeQueue.clear();
    }
    if (_mediaSink) {
        auto status = COMPLETION_STATUS(_mediaSink->Shutdown());
        _mediaSink.Reset();
        return status;
    }
    return {};
}

CompletionStatus MFVideoEncoder::reconfigureWriter(UINT32 newWidth, UINT32 newHeight,
                                                   UINT32 newTargetBps, UINT32 newFrameRate)
{
    auto status = releaseWriter();
    if (!status) {
        return status;
    }
    return initWriter(newWidth, newHeight, newTargetBps, newFrameRate, qpMax());
}

void MFVideoEncoder::onEncoded(CComPtr<IMFSample> sample)
{
    if (sample) {
        LONGLONG sampleTimestamp = 0LL;
        auto status = COMPLETION_STATUS(sample->GetSampleTime(&sampleTimestamp));
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return;
        }
        VideoFrameInfo info;
        // pop the attributes for this sample:
        // this must be done even if the frame is discarded later, or the queue will clog
        if (!_sampleAttributeQueue.pop(sampleTimestamp, info)) {
            // No point in processing a frame that doesn't have correct attributes.
            return;
        }
        // copy raw output sample data to encoded image
        CComPtr<IMFMediaBuffer> data;
        status = COMPLETION_STATUS(sample->GetBufferByIndex(0, &data));
        if (!status) {
            status = COMPLETION_STATUS(sample->ConvertToContiguousBuffer(&data));
        }
        if (!status) {
            RTC_LOG(LS_ERROR) << status;
            return;
        }
        const bool keyFrame = TRUE == ::MFGetAttributeUINT32(sample, MFSampleExtension_CleanPoint, FALSE);
        info.markFinishTimestamp();
        auto image = createEncodedImage(keyFrame, data, encoderQp(sample));
        if (!image) {
            RTC_LOG(LS_ERROR) << image.status();
            return;
        }
        image->capture_time_ms_ = info.renderTimeMs();
        image->ntp_time_ms_ = info.ntpTimeMs();
        image->_encodedWidth = info.width();
        image->_encodedHeight = info.height();
        image->rotation_ = info.rotation();
        image->SetRtpTimestamp(info.timestampRtpMs());
        image->SetEncodeTime(info.startTimestampMs(), info.finishTimestampMs());
        sendEncodedImage(keyFrame, image.moveValue());
    }
}

MFVideoEncoder::EncodedBuffer::EncodedBuffer(MFMediaBufferLocker locker)
    : _locker(std::move(locker))
{
}

} // namespace LiveKitCpp
#endif