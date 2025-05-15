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
#include "LibyuvImport.h"
#include <Mfapi.h>
#include <Mferror.h>
#include <rtc_base/time_utils.h>

namespace LiveKitCpp 
{

MFVideoEncoder::MFVideoEncoder(const webrtc::SdpVideoFormat& format, webrtc::CodecSpecificInfo codecSpecificInfo)
    : VideoEncoder(format, std::move(codecSpecificInfo), false)
{
}

MFVideoEncoder::~MFVideoEncoder()
{
    MFVideoEncoder::destroySession();
}

bool MFVideoEncoder::hardwareAccelerated() const
{
    if (_pipeline) {
        return _pipeline.hardwareAccellerated() || _pipeline.directXAccelerationSupported();
    }
    return VideoEncoder::hardwareAccelerated();
}

int32_t MFVideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t r = VideoEncoder::InitEncode(codecSettings, encoderSettings);
    if (WEBRTC_VIDEO_CODEC_OK == r) {
        auto pipeline = createPipeline(codecSettings->width, codecSettings->height);
        if (!pipeline) {
            logError(pipeline.moveStatus());
            r = WEBRTC_VIDEO_CODEC_ERROR;
        } else {
            _currentWidth = codecSettings->width;
            _currentHeight = codecSettings->height;
            _pipeline = pipeline.moveValue();
        }
    }
    return r;
}

int32_t MFVideoEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes)
{
    if (hasEncodeCompleteCallback() && _pipeline) {
        if (dropNextFrame()) {
            if (_pipeline.started()) {
                _pipeline.drain();
                _lastFrameDropped = true;
            }
            return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
        }
        if (const auto inputBuffer = _pipeline.createMediaBuffer(frame)) {
            const bool keyFrame = keyFrameRequested(frameTypes) /* ||
                                  frame.width() != _currentWidth ||
                                  frame.height() != _currentHeight*/;
            CompletionStatus hr;
            if (keyFrame) {
                hr = acceptInputResolution(frame);
                if (hr) {
                    hr = fillMpegHeaderData(_pipeline);
                }
            }
            DWORD outputFramesCount = 0UL;
            if (hr) {
                if (!_pipeline.started()) {
                    hr = _pipeline.start();
                }
                if (hr) {
                    if (_pipeline.sync()) {
                        const auto startTimeMs = webrtc::TimeMillis();
                        hr = enqueueInput(keyFrame, frame, startTimeMs, inputBuffer);
                        if (hr) {
output:
                            auto framesCount = processOutput();
                            if (framesCount) {
                                outputFramesCount = framesCount.value();
                            } else {
                                switch (framesCount.code()) {
                                    case MF_E_TRANSFORM_NEED_MORE_INPUT:
                                        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
                                    case MF_E_TRANSFORM_STREAM_CHANGE:
                                        break;
                                    default:
                                        hr = framesCount.moveStatus();
                                        break;
                                }
                            }
                        } else if (MF_E_NOTACCEPTING == hr.code()) {
                            // According to MSDN, if encoder returns MF_E_NOTACCEPTING, we need to try
                            // processing the output. This error indicates that encoder does not accept
                            // any more input data.
                            processOutput();
                            hr = enqueueInput(keyFrame, frame, startTimeMs, inputBuffer);
                            if (hr) {
                                goto output;
                            }
                        }

                    } else {
                        assert(false); // async not yet implemented
                    }
                }
            }
            if (logError(std::move(hr)).ok()) {
                return outputFramesCount ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_NO_OUTPUT;
            }
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
    }
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
}

webrtc::VideoEncoder::EncoderInfo MFVideoEncoder::GetEncoderInfo() const
{
    auto encoderInfo = VideoEncoder::GetEncoderInfo();
    addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type::kNV12, encoderInfo);
    return encoderInfo;
}

std::optional<UINT32> MFVideoEncoder::encoderQp(const CComPtr<IMFSample>& sample)
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

CompletionStatusOr<MFVideoEncoderPipeline> MFVideoEncoder::createPipeline(UINT32 width, UINT32 height)
{
    auto pipeline = MFVideoEncoderPipeline::create(type(), width, height, currentFramerate(), currentBitrate());
    if (pipeline) {
        auto hr = fillMpegHeaderData(pipeline.value());
        if (hr) {
            logWarning(pipeline->setVideoMaxQP(qpMax()));
            logWarning(pipeline->setLowLatencyMode(true));
            logWarning(pipeline->setRealtimeContent(true));
            logWarning(pipeline->setVideoContentType(realtimeMode()));
            logWarning(pipeline->setCompressedFramerate(currentFramerate()));
            logWarning(pipeline->setCommonMinBitRate(minBitrateBps()));
            logWarning(pipeline->setCommonMaxBitRate(maxBitrateBps()));
            logWarning(pipeline->setCommonMeanBitRate(currentBitrate()));
            //logWarning(pipeline->setAdaptiveMode(realtimeMode(), true));
            return pipeline;
        }
        return hr;
    }
    return pipeline;
}

CompletionStatus MFVideoEncoder::destroySession()
{
    if (_pipeline) {
        logWarning(_pipeline.stop());
        _pipeline = {};
        _currentWidth = _currentHeight = 0U;
        while (!_attributeQueue.empty()) {
            _attributeQueue.pop();
        }
        _lastFrameDropped = false;
    }
    return VideoEncoder::destroySession();
}

CompletionStatus MFVideoEncoder::setEncoderBitrate(uint32_t bitrateBps)
{
    if (_pipeline) {
        return _pipeline.setCommonMeanBitRate(bitrateBps);
    }
    return {};
}

CompletionStatus MFVideoEncoder::setEncoderFrameRate(uint32_t frameRate)
{
    if (_pipeline) {
        return _pipeline.setCompressedFramerate(frameRate);
    }
    return {};
}

CompletionStatus MFVideoEncoder::fillMpegHeaderData(const MFVideoEncoderPipeline& pipeline)
{
    if (pipeline) {
        auto hdr = pipeline.mpegSequenceHeader();
        if (hdr) {
            return processMpegHeaderData(hdr.moveValue());
        }
        return hdr.moveStatus();
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

CompletionStatus MFVideoEncoder::enqueueInput(bool keyFrame, const webrtc::VideoFrame& frame,
                                              int64_t startTimeMs, const CComPtr<IMFMediaBuffer>& inputBuffer)
{
    if (_pipeline) {
        if (inputBuffer) {
            // commit video frame into encoder
            // create a sample from media buffer
            auto inputSample = createSample(inputBuffer);
            if (inputSample) {
                auto hr = _pipeline.setSampleTimeMetrics(inputSample.value(), frame);
                if (hr && keyFrame) {
                    hr = _pipeline.setForceKeyFrame(true);
                }
                if (hr) {
                    if (_lastFrameDropped) {
                        logError(COMPLETION_STATUS(inputSample.value()->SetUINT32(MFSampleExtension_Discontinuity, TRUE)));
                        _lastFrameDropped = false;
                        _pipeline.flush();
                    }
                    hr = _pipeline.processInput(inputSample.value());
                    if (hr) {
                        _attributeQueue.push(std::make_pair(_pipeline.lastTimestampHns(), VideoFrameInfo(frame)));
                    }
                }
                return hr;
            }
            return inputSample.moveStatus();
        }
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

CompletionStatus MFVideoEncoder::acceptInputResolution(const webrtc::VideoFrame& frame)
{
    if (_pipeline) {
        if (_currentWidth != frame.width() || _currentHeight != frame.height()) {
            auto hr = _pipeline.setCompressedFrameSize(frame.width(), frame.height());
            if (hr) {
                _currentWidth = frame.width();
                _currentHeight = frame.height();
            }
            return hr;
        }
        return {};
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

std::optional<VideoFrameInfo> MFVideoEncoder::popVideoFrameInfo(LONGLONG timestampNhs)
{
    while (!_attributeQueue.empty()) {
        auto& entry = _attributeQueue.front();
        if (entry.first > timestampNhs) {
            return entry.second;
        } else if (entry.first == timestampNhs) {
            auto attributes = std::move(entry.second);
            _attributeQueue.pop();
            return attributes;
        } else {
            _attributeQueue.pop();
        }
    }
    return std::nullopt;
}

CompletionStatusOr<UINT64> MFVideoEncoder::processOutput()
{
    if (_pipeline) {
        CompletionStatus hr;
        UINT64 sentFramesCount = 0UL;
        while (hr && hasEncodeCompleteCallback()) {
            auto outputStatus = _pipeline.compressedStatus();
            if (outputStatus) {
                if (MFT_OUTPUT_STATUS_SAMPLE_READY == outputStatus.value()) {
                    auto sample = _pipeline.createSampleWitMemoryBuffer(false);
                    if (sample) {
                        sample = _pipeline.processOutput(sample.value());
                        if (sample) {
                            if (sample.value()) {
                                LONGLONG timestampNhs = 0LL;
                                hr = COMPLETION_STATUS(sample.value()->GetSampleTime(&timestampNhs));
                                if (hr) {
                                    // pop the attributes for this sample:
                                    // this must be done even if the frame is discarded later, or the queue will clog
                                    auto info = popVideoFrameInfo(timestampNhs);
                                    // no point in processing a frame that doesn't have correct attributes
                                    if (info.has_value()) {
                                        // copy raw output sample data to encoded image
                                        CComPtr<IMFMediaBuffer> data;
                                        hr = COMPLETION_STATUS(sample.value()->ConvertToContiguousBuffer(&data));
                                        if (hr) {
                                            const bool keyFrame = TRUE == ::MFGetAttributeUINT32(sample.value(),
                                                                                                 MFSampleExtension_CleanPoint,
                                                                                                 FALSE);
                                            info->markFinishTimestamp();
                                            hr = sendEncoded(data, keyFrame, info.value(), encoderQp(sample.value()));
                                            if (hr) {
                                                ++sentFramesCount;
                                            }
                                        }
                                    }
                                }
                            } else {
                                break; // no output
                            }
                        } else {
                            hr = sample.moveStatus();
                        }
                    } else {
                        hr = sample.moveStatus();
                    }
                } else {
                    break;
                }
            } else {
                hr = outputStatus.moveStatus();
            }
        }
        if (hr) {
            return sentFramesCount;
        }
        return hr;
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

CompletionStatus MFVideoEncoder::sendEncoded(const CComPtr<IMFMediaBuffer>& data, bool keyFrame,
                                             const VideoFrameInfo& frameInfo,
                                             const std::optional<UINT32>& encodedQp)
{
    auto encodedImage = createEncodedImage(keyFrame, data, encodedQp);
    if (encodedImage) {
        encodedImage->capture_time_ms_ = frameInfo.renderTimeMs();
        encodedImage->_encodedWidth = frameInfo.width();
        encodedImage->_encodedHeight = frameInfo.height();
        encodedImage->rotation_ = frameInfo.rotation();
        encodedImage->SetRtpTimestamp(frameInfo.timestampRtpMs());
        encodedImage->SetEncodeTime(frameInfo.startTimestampMs(), frameInfo.finishTimestampMs());
        sendEncodedImage(keyFrame, encodedImage.moveValue());
        return {};
    }
    return encodedImage.moveStatus();
}

class MFVideoEncoder2::EncodedBuffer : public webrtc::EncodedImageBufferInterface
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

MFVideoEncoder2::MFVideoEncoder2(const webrtc::SdpVideoFormat& format, webrtc::CodecSpecificInfo codecSpecificInfo)
    : VideoEncoder(format, std::move(codecSpecificInfo), false)
    , _framesPool(VideoFrameBufferPoolSource::create())
{
    _inputFramesTimeline.setSetupDuration(true);
}

int32_t MFVideoEncoder2::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
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
        auto status = initWriter(codecSettings->width, codecSettings->height, targetBps, maxFramerate(), qpMax());
        if (!status) {
            logError(std::move(status));
            r = WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    return r;
}

int32_t MFVideoEncoder2::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes)
{
    if (!_sinkWriter) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    const auto now = webrtc::TimeMillis();
    const UINT32 width = frame.width();
    const UINT32 height = frame.height();
    // Reset the encoder configuration if necessary.
    const auto resChanged = width != _width || height != _height;
    const auto shouldChangeRateNow = _rateChangeRequested && (now - _lastRateChangeTimeMs) > _minIntervalBetweenRateChangesMs;
    if (resChanged || shouldChangeRateNow) {
        const auto status = logError(reconfigureWriter(width, height, currentBitrate(), currentFramerate()));
        if (!status) {
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
        if (!logError(std::move(status))) {
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
        auto status = COMPLETION_STATUS(_sinkWriter->SendStreamTick(_streamIndex, _inputFramesTimeline.lastTimestampHns()));
        return logError(std::move(status)).ok() ? WEBRTC_VIDEO_CODEC_OK : WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    auto status = COMPLETION_STATUS(_sinkWriter->WriteSample(_streamIndex, sample));
    if (!logError(std::move(status)).ok()) {
        return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    if (0U == ++_framesCount % currentFramerate()) {
        // some threads online mention this is useful to do regularly
        status = COMPLETION_STATUS(_sinkWriter->NotifyEndOfSegment(_streamIndex));
        if (!logError(std::move(status)).ok()) {
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

webrtc::VideoEncoder::EncoderInfo MFVideoEncoder2::GetEncoderInfo() const
{
    auto encoderInfo = VideoEncoder::GetEncoderInfo();
    addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type::kNV12, encoderInfo);
    return encoderInfo;
}

CompletionStatus MFVideoEncoder2::destroySession()
{
    auto status = releaseWriter();
    if (!status) {
        return status;
    }
    _inputFramesTimeline.reset();
    _lastInputFrameDropped = false;
    return VideoEncoder::destroySession();
}

CompletionStatus MFVideoEncoder2::setEncoderBitrate(uint32_t bitrateBps)
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

CompletionStatus MFVideoEncoder2::setEncoderFrameRate(uint32_t frameRate)
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

CompletionStatusOr<webrtc::EncodedImage> MFVideoEncoder2::createEncodedImage(bool keyFrame,
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

std::optional<UINT32> MFVideoEncoder2::encoderQp(IMFSample* sample)
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

CComPtr<IMFSample> MFVideoEncoder2::fromVideoFrame(const webrtc::VideoFrame& frame)
{
    CComPtr<IMFMediaBuffer> buffer = MFEncoderInputMediaBuffer::create(frame, {_framesPool});
    if (buffer) {
        auto result = createSample(buffer);
        if (!result) {
            logError(result.moveStatus());
            return {};
        }
        auto sample = result.value();
        auto status = _inputFramesTimeline.setTimeMetrics(frame, sample);
        if (!status) {
            logError(std::move(status));
            return {};
        }
        if (_lastInputFrameDropped || dropNextFrame()) {
            CComPtr<IMFAttributes> sampleAttributes;
            status = COMPLETION_STATUS(sample.QueryInterface<IMFAttributes>(&sampleAttributes));
            if (!status) {
                logError(std::move(status));
                return {};
            }
            status = COMPLETION_STATUS(sampleAttributes->SetUINT32(MFSampleExtension_Discontinuity, TRUE));
            if (!status) {
                logError(std::move(status));
                return {};
            }
            _lastInputFrameDropped = false;
        }
        _sampleAttributeQueue.push(_inputFramesTimeline.lastTimestampHns(), VideoFrameInfo(frame));
        return sample;
    }
    return {};
}

CompletionStatus MFVideoEncoder2::initWriter(UINT32 width, UINT32 height, UINT32 targetBps,
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
    status = COMPLETION_STATUS(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, compressedFormat()));
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

CompletionStatus MFVideoEncoder2::releaseWriter()
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

CompletionStatus MFVideoEncoder2::reconfigureWriter(UINT32 newWidth, UINT32 newHeight,
                                                    UINT32 newTargetBps, UINT32 newFrameRate)
{
    auto status = releaseWriter();
    if (!status) {
        return status;
    }
    return initWriter(newWidth, newHeight, newTargetBps, newFrameRate, qpMax());
}

const GUID& MFVideoEncoder2::compressedFormat() const
{
    switch (type()) {
        case webrtc::VideoCodecType::kVideoCodecVP8:
            return MFVideoFormat_VP80;
        case webrtc::VideoCodecType::kVideoCodecVP9:
            return MFVideoFormat_VP90;
        case webrtc::VideoCodecType::kVideoCodecAV1:
            return MFVideoFormat_AV1;
        case webrtc::VideoCodecType::kVideoCodecH264:
            return MFVideoFormat_H264;
        case webrtc::VideoCodecType::kVideoCodecH265:
            return MFVideoFormat_H265;
        default:
            break;
    }
    return GUID_NULL;
}

void MFVideoEncoder2::onEncoded(CComPtr<IMFSample> sample)
{
    if (sample) {
        LONGLONG sampleTimestamp = 0LL;
        auto status = COMPLETION_STATUS(sample->GetSampleTime(&sampleTimestamp));
        if (!status) {
            logError(std::move(status));
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
            logError(std::move(status));
            return;
        }
        const bool keyFrame = TRUE == ::MFGetAttributeUINT32(sample, MFSampleExtension_CleanPoint, FALSE);
        info.markFinishTimestamp();
        auto image = createEncodedImage(keyFrame, data, encoderQp(sample));
        if (!image) {
            logError(image.moveStatus());
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

MFVideoEncoder2::EncodedBuffer::EncodedBuffer(MFMediaBufferLocker locker)
    : _locker(std::move(locker))
{
}

} // namespace LiveKitCpp
#endif