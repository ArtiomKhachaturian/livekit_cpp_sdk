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
#include "MFVideoDecoder.h"
#include "MFCommon.h"
#include "MFDecoderInputMediaBuffer.h"
#include "Utils.h"
#include "VideoUtils.h"
#include <rtc_base/time_utils.h>
#include <Mferror.h>
#include <mfapi.h>

namespace LiveKitCpp 
{

MFVideoDecoder::MFVideoDecoder(const webrtc::SdpVideoFormat& format)
    : VideoDecoder(format)
{
}

MFVideoDecoder::~MFVideoDecoder()
{
}

bool MFVideoDecoder::hardwareAccelerated() const
{
    if (_pipeline) {
        return _pipeline.hardwareAccellerated() || _pipeline.dxvaAccelerated();
    }
    return VideoDecoder::hardwareAccelerated();
}

bool MFVideoDecoder::Configure(const Settings& settings)
{
    if (VideoDecoder::Configure(settings)) {
        UINT32 maxCodedWidth = 0U, maxCodedHeight = 0U;
        bool hwa = true;
        if (settings.max_render_resolution().Valid()) {
            maxCodedWidth = settings.max_render_resolution().Width();
            maxCodedHeight = settings.max_render_resolution().Height();
            if (hwa && webrtc::VideoCodecType::kVideoCodecH264 == settings.codec_type()) {
                hwa = maxCodedWidth > 132U && maxCodedHeight >= 132;
            }
        }
        CompletionStatus status;
        auto pipeline = MFVideoDecoderPipeline::create(hwa, settings.codec_type(), maxCodedWidth, maxCodedHeight);
        if (pipeline) {
            _currentWidth = maxCodedWidth;
            _currentHeight = maxCodedHeight;
            logWarning(pipeline->setLowLatencyMode(true));
            if (!pipeline->hardwareAccellerated()) {
                const auto threads = maxDecodingThreads(maxCodedWidth, maxCodedHeight,
                                                        settings.number_of_cores());
                logWarning(pipeline->setNumWorkerThreads(threads));
            }
            status = logError(pipeline->selectUncompressedMediaType());
            if (status) {
                status = logError(pipeline->start());
                if (status) {
                    _pipeline = pipeline.moveValue();
                }
            }
        } else {
            status = logError(pipeline.moveStatus());
        }
        return status.ok();
    }
    return false;
}

int32_t MFVideoDecoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs)
{
    int32_t result = WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    if (_pipeline) {
        if (inputImage.data() && inputImage.size()) {
            result = WEBRTC_VIDEO_CODEC_OK; // preliminary status
            if (_requestKeyFrame) {
                if (webrtc::VideoFrameType::kVideoFrameKey != inputImage._frameType) {
                    result = WEBRTC_VIDEO_CODEC_ERROR;
                } else {
                    _requestKeyFrame = false;
                }
            }
            if (WEBRTC_VIDEO_CODEC_OK == result) {
                CompletionStatus status;
                if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
                    const auto realtime = webrtc::VideoContentType::UNSPECIFIED == inputImage.content_type_;
                    logWarning(_pipeline.setRealtimeContent(realtime));
                    status = setCompressedFrameSize(inputImage._encodedWidth, inputImage._encodedHeight);
                }
                if (WEBRTC_VIDEO_CODEC_OK == result && status) {
                    status = enqueueFrame(inputImage, missingFrames);
                    if (MF_E_NOTACCEPTING == status.code()) {
                        // for robustness (shouldn't happen),
                        // flush any old MF data blocking the new frames
                        _pipeline.flush();
                        if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
                            status = enqueueFrame(inputImage, missingFrames);
                        } else {
                            result = WEBRTC_VIDEO_CODEC_OK_REQUEST_KEYFRAME;
                            _requestKeyFrame = true;
                            status = {};
                        }
                    }
                    if (WEBRTC_VIDEO_CODEC_OK == result && status) {
                        // flush any decoded samples resulting from new frame, invoking callback
                        status = flushFrames(inputImage);
                        if (MF_E_TRANSFORM_STREAM_CHANGE == status.code()) {
                            status = _pipeline.selectUncompressedMediaType();
                            if (status) {
                                status = flushFrames(inputImage);
                            }
                        }
                    }
                }
                if (!status) {
                    logError(status);
                    result = WEBRTC_VIDEO_CODEC_ERROR;
                }
            }
        } else {
            result = WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
    }
    return result;
}

CompletionStatus MFVideoDecoder::setCompressedFrameSize(UINT32 width, UINT32 height)
{
    if (_pipeline) {
        if (_currentWidth != width || _currentHeight != height) {
            auto hr = _pipeline.setCompressedFrameSize(width, height);
            if (hr) {
                _currentWidth = width;
                _currentHeight = height;
            }
            return hr;
        }
        return {};
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

void MFVideoDecoder::destroySession()
{
    _pipeline = {};
    _requestKeyFrame = false;
    _currentWidth = _currentHeight = 0;
}

CompletionStatus MFVideoDecoder::enqueueFrame(const webrtc::EncodedImage& inputImage, bool missingFrames)
{
    if (_pipeline) {
        if (const auto mediaBuffer = MFDecoderInputMediaBuffer::create(inputImage.GetEncodedData())) {
            // create a sample from media buffer
            auto sample = createSample(mediaBuffer);
            if (sample) {
                auto hr = _pipeline.setSampleTimeMetrics(sample.value(), inputImage);
                if (hr) {
                    CComPtr<IMFAttributes> attributes;
                    if (SUCCEEDED(sample.value()->QueryInterface(&attributes))) {
                        if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
                            attributes->SetUINT32(MFSampleExtension_CleanPoint, TRUE);
                        }
                        if (missingFrames) {
                            attributes->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
                        }
                    }
                    hr = _pipeline.processInput(sample.value());
                }
                return hr;
            }
            return sample.moveStatus();
        }
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

CompletionStatus MFVideoDecoder::flushFrames(const webrtc::EncodedImage& inputImage)
{
    if (_pipeline) {
        CompletionStatus hr;
        while (hr && hasDecodeCompleteCallback()) {
            auto outputStatus = _pipeline.uncompressedStatus();
            if (outputStatus) {
                if (MFT_OUTPUT_STATUS_SAMPLE_READY == outputStatus.value()) {
                    auto sample = _pipeline.createSampleWitMemoryBuffer(false);
                    if (sample) {
                        sample = _pipeline.processOutput(sample.value());
                        if (MF_E_TRANSFORM_NEED_MORE_INPUT == sample.code() ||
                            MF_E_TRANSFORM_STREAM_CHANGE == sample.code()) {
                            break; // it's OK for us
                        } else if (sample) {
                            hr = sendDecodedSample(inputImage, sample.value());
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
        return hr;
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

CompletionStatus MFVideoDecoder::sendDecodedSample(const webrtc::EncodedImage& inputImage,
                                                   const CComPtr<IMFSample>& sample)
{
    if (_pipeline) {
        if (sample) {
            // copy raw output sample data to video frame buffer.
            CComPtr<IMFMediaBuffer> srcBuffer;
            auto hr = COMPLETION_STATUS(sample->ConvertToContiguousBuffer(&srcBuffer));
            if (hr) {
                UINT32 width = 0U, height = 0U;
                if (const auto aperture = _pipeline.minimumDisplayAperture()) {
                    width = aperture->Area.cx;
                    height = aperture->Area.cy;
                }
                auto nv12buff = _pipeline.createBuffer(srcBuffer, width, height);
                if (nv12buff) {
                    auto frame = createVideoFrame(nv12buff.value(), 
                                                  webrtc::VideoRotation::kVideoRotation_0, 
                                                  inputImage.NtpTimeMs() * rtc::kNumMicrosecsPerMillisec);
                    if (frame.has_value()) {
                        frame->set_rtp_timestamp(inputImage.RtpTimestamp());
                        sendDecodedImage(frame.value(), std::nullopt, lastSliceQp(inputImage));
                    } else {
                        hr = COMPLETION_STATUS(E_FAIL);
                    }
                } else {
                    hr = nv12buff.moveStatus();
                }
            }
            return hr;
        }
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

} // namespace LiveKitCpp