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
#include "MFCommon.h"
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
                        const auto startTimeMs = rtc::TimeMillis();
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
    auto pipeline = MFVideoEncoderPipeline::create(true, type(), width, height, currentFramerate(), currentBitrate());
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

void MFVideoEncoder::destroySession()
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
    VideoEncoder::destroySession();
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
                        FrameInfo frameInfo;
                        frameInfo._startTimestampMs = startTimeMs;
                        frameInfo._renderTimeMs = frame.render_time_ms();
                        frameInfo._timestampUs = frame.timestamp_us();
                        frameInfo._timestampRtp = frame.rtp_timestamp();
                        frameInfo._width = frame.width();
                        frameInfo._height = frame.height();
                        frameInfo._rotation = frame.rotation();
                        _attributeQueue.push(std::make_pair(_pipeline.lastTimestampHns(), std::move(frameInfo)));
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

std::optional<MFVideoEncoder::FrameInfo> MFVideoEncoder::popVideoFrameInfo(LONGLONG timestampNhs)
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
                                            info->_finishTimestampMs = rtc::TimeMillis();
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
                                             const FrameInfo& frameInfo,
                                             const std::optional<UINT32>& encodedQp)
{
    auto encodedImage = createEncodedImage(keyFrame, data, encodedQp);
    if (encodedImage) {
        encodedImage->capture_time_ms_ = frameInfo._renderTimeMs;
        encodedImage->_encodedWidth = frameInfo._width;
        encodedImage->_encodedHeight = frameInfo._height;
        encodedImage->rotation_ = frameInfo._rotation;
        encodedImage->SetRtpTimestamp(frameInfo._timestampRtp);
        encodedImage->SetEncodeTime(frameInfo._startTimestampMs, frameInfo._finishTimestampMs);
        sendEncodedImage(keyFrame, encodedImage.moveValue());
    }
    return encodedImage.moveStatus();
}

} // namespace LiveKitCpp