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
#include "VideoEncoder.h"
#include "SafeObjAliases.h"
#include "Utils.h"
#include <common_video/include/bitrate_adjuster.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <modules/video_coding/utility/simulcast_utility.h>

namespace
{

float maxAllowedFrameRateH264(webrtc::H264Level level, uint16_t width, uint16_t height, float maxFrameRate);

template <class TCodecSource>
inline float maxAllowedFrameRate(webrtc::H264Level level, const TCodecSource* source)
{
    if (source) {
        return maxAllowedFrameRateH264(level, static_cast<uint16_t>(source->width),
                                       static_cast<uint16_t>(source->height),
                                       source->maxFramerate);
    }
    return 0.f;
}

} // namespace

namespace LiveKitCpp
{

class VideoEncoder::BitrateAdjuster
{
public:
    BitrateAdjuster(float minAdjustedBitratePct = VideoEncoder::minAdjustedBitratePct(),
                    float maxAdjustedBitratePct = VideoEncoder::maxAdjustedBitratePct());
    ~BitrateAdjuster() { _impl({}); }
    void reset();
    // return true if changed
    bool setTargetBitrate(uint32_t bitrate);
    uint32_t adjustedBitrate() const;
    std::optional<uint32_t> estimatedBitrate();
    void update(size_t frameSize);
    bool hasPendingBitrateUpdates() { return _hasPendingBitrateUpdates.exchange(false); }
private:
    std::shared_ptr<webrtc::BitrateAdjuster> createImpl() const;
    uint32_t targetBitrate() const { return _targetBitrateBps.load(std::memory_order_relaxed); }
private:
    const float _minAdjustedBitratePct;
    const float _maxAdjustedBitratePct;
    Bricks::SafeUniquePtr<webrtc::BitrateAdjuster> _impl;
    std::atomic<uint32_t> _targetBitrateBps = 0U;
    std::atomic_bool _hasPendingBitrateUpdates = false;
};

VideoEncoder::VideoEncoder(bool hardwareAccelerated, webrtc::CodecSpecificInfo codecSpecificInfo,
                           bool useTrustedBitrateController,
                           const std::shared_ptr<Bricks::Logger>& logger)
    : GenericCodec<webrtc::VideoEncoder>(hardwareAccelerated, logger)
    , _codecSpecificInfo(std::move(codecSpecificInfo))
    , _bitrateAdjuster(useTrustedBitrateController ? std::make_unique<BitrateAdjuster>() : std::unique_ptr<BitrateAdjuster>())
{
}

VideoEncoder::~VideoEncoder()
{
}

int32_t VideoEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    if (codecSettings && _codecSpecificInfo.codecType == codecSettings->codecType &&
        codecSettings->maxFramerate > 0U && codecSettings->width > 1U && codecSettings->height > 1U) {
        // simulcast is not supported
        if (webrtc::SimulcastUtility::NumberOfSimulcastStreams(*codecSettings) <= 1) {
            _mode = codecSettings->mode;
            _qpMax = codecSettings->qpMax;
            _minBitrate = codecSettings->minBitrate * 1000;
            _maxBitrate = codecSettings->maxBitrate * 1000;
            _maxFramerate = codecSettings->maxFramerate;
            if (codecSettings->startBitrate) {
                setTargetBitrate(codecSettings->startBitrate * 1000);
            } else if (_minBitrate) {
                setTargetBitrate(_minBitrate);
            } else {
                setTargetBitrate(codecSettings->width * codecSettings->height * 2U);
            }
            setCurrentFramerate(codecSettings->maxFramerate);
            return WEBRTC_VIDEO_CODEC_OK;
        }
        return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
    }
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
}

int32_t VideoEncoder::Release()
{
    // Need to destroy so that the session is invalidated and won't use the
    // callback anymore. Do not remove callback until the session is invalidated
    // since async encoder callbacks can occur until invalidation.
    destroySession();
    _currentBitrate = _minBitrate = _maxBitrate = 0U;
    _currentFramerate = _maxFramerate = 0U;
    return RegisterEncodeCompleteCallback(nullptr); // nullptr is acceptable
}

int32_t VideoEncoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback)
{
    _callback(callback);
    return WEBRTC_VIDEO_CODEC_OK;
}

void VideoEncoder::SetRates(const RateControlParameters& parameters)
{
    setTargetBitrate(parameters.bitrate.get_sum_bps());
    setCurrentFramerate(parameters.framerate_fps);
}

webrtc::VideoEncoder::EncoderInfo VideoEncoder::GetEncoderInfo() const
{
    webrtc::VideoEncoder::EncoderInfo encoderInfo;
    encoderInfo.supports_native_handle = true; // RGB buffers
    encoderInfo.implementation_name = mediaBackendName();
    encoderInfo.is_hardware_accelerated = hardwareAccelerated();
    encoderInfo.has_trusted_rate_controller = nullptr != _bitrateAdjuster.get();
    switch (_codecSpecificInfo.codecType) {
        case webrtc::VideoCodecType::kVideoCodecH264:
            encoderInfo.requested_resolution_alignment = 1;
            encoderInfo.apply_alignment_to_all_simulcast_layers = false;
            encoderInfo.scaling_settings = webrtc::VideoEncoder::ScalingSettings(lowH264QpThreshold(),
                                                                                 highH264QpThreshold());
            break;
        default:
            break;
    }
    return encoderInfo;
}

void VideoEncoder::destroySession()
{
    _dropNextFrame = false;
    if (_bitrateAdjuster) {
        _bitrateAdjuster->reset();
    }
}

bool VideoEncoder::hasEncodeCompleteCallback() const
{
    LOCK_READ_SAFE_OBJ(_callback);
    return nullptr != _callback.constRef();
}

void VideoEncoder::dropEncodedImage(webrtc::EncodedImageCallback::DropReason reason) const
{
    LOCK_READ_SAFE_OBJ(_callback);
    if (const auto callback = _callback.constRef()) {
        callback->OnDroppedFrame(reason);
    }
}

void VideoEncoder::sendEncodedImage(bool keyFrame, webrtc::EncodedImage encodedImage)
{
    LOCK_READ_SAFE_OBJ(_callback);
    if (const auto callback = _callback.constRef()) {
        switch (mode()) {
            case webrtc::VideoCodecMode::kScreensharing:
                encodedImage.content_type_ = webrtc::VideoContentType::SCREENSHARE;
                break;
            default:
                encodedImage.content_type_ = webrtc::VideoContentType::UNSPECIFIED;
                break;
        }
        if (keyFrame) {
            encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameKey;
        } else {
            encodedImage._frameType = webrtc::VideoFrameType::kVideoFrameDelta;
        }
        encodedImage.timing_.flags = webrtc::VideoSendTiming::kInvalid;
        const auto result = callback->OnEncodedImage(encodedImage, &_codecSpecificInfo);
        if (webrtc::EncodedImageCallback::Result::Error::OK == result.error) {
            _dropNextFrame = result.drop_next_frame;
            if (_bitrateAdjuster) {
                _bitrateAdjuster->update(encodedImage.size());
            }
        }
    }
}

bool VideoEncoder::dropNextFrame()
{
    const auto drop = _dropNextFrame.exchange(false);
    if (_bitrateAdjuster && _bitrateAdjuster->hasPendingBitrateUpdates()) {
        setCurrentBitrate(_bitrateAdjuster->adjustedBitrate());
    }
    return drop;
}

webrtc::VideoCodec VideoEncoder::toH264Settings(const webrtc::VideoCodec& settings,
                                                webrtc::H264Level level)
{
    webrtc::VideoCodec h264;
    h264.codecType = webrtc::VideoCodecType::kVideoCodecH264;
    h264.width = settings.width;
    h264.height = settings.height;
    h264.startBitrate = settings.startBitrate;
    h264.maxBitrate = settings.maxBitrate;
    h264.minBitrate = settings.minBitrate;
    h264.maxFramerate = static_cast<uint32_t>(maxAllowedFrameRate(level, &settings) + 0.5f);
    h264.active = settings.active;
    h264.qpMax = settings.qpMax;
    h264.numberOfSimulcastStreams = settings.numberOfSimulcastStreams;
    for (int i = 0; i < webrtc::kMaxSimulcastStreams; ++i) {
        h264.simulcastStream[i] = settings.simulcastStream[i];
        h264.simulcastStream[i].maxFramerate = maxAllowedFrameRate(level, &h264.simulcastStream[i]);
    }
    for (int i = 0; i < webrtc::kMaxSpatialLayers; ++i) {
        h264.spatialLayers[i] = settings.spatialLayers[i];
        h264.spatialLayers[i].maxFramerate = maxAllowedFrameRate(level, &h264.spatialLayers[i]);
    }
    h264.mode = settings.mode;
    h264.expect_encode_from_texture = settings.expect_encode_from_texture;
    h264.timing_frame_thresholds = settings.timing_frame_thresholds;
    h264.legacy_conference_mode = settings.legacy_conference_mode;
    //h264.H264()->frameDroppingOn = settings.H264().frameDroppingOn;
    h264.H264()->keyFrameInterval = settings.H264().keyFrameInterval;
    h264.H264()->numberOfTemporalLayers = settings.H264().numberOfTemporalLayers;
    return h264;
}

bool VideoEncoder::keyFrameRequested(const std::vector<webrtc::VideoFrameType>* frameTypes)
{
    if (frameTypes && !frameTypes->empty()) {
        for (size_t i = 0, end = frameTypes->size(); i < end; ++i) {
            if (webrtc::VideoFrameType::kVideoFrameKey == frameTypes->at(i)) {
                return true;
            }
        }
    }
    return false;
}

void VideoEncoder::addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type type, EncoderInfo& info)
{
    auto& formats = info.preferred_pixel_formats;
    if (formats.end() == std::find(formats.begin(), formats.end(), type)) {
        formats.push_back(type);
    }
}

void VideoEncoder::setTargetBitrate(uint32_t bps)
{
    if (_bitrateAdjuster && _bitrateAdjuster->setTargetBitrate(bps)) {
        setCurrentBitrate(_bitrateAdjuster->adjustedBitrate());
    } else {
        setCurrentBitrate(bps);
    }
}

void VideoEncoder::setCurrentBitrate(uint32_t bps)
{
    if (_minBitrate && _maxBitrate) {
        bps = bound(_minBitrate.load(), bps, _maxBitrate.load());
        if (bps != _currentBitrate) {
            // update the bitrate if needed
            auto result = setEncoderBitrate(bps);
            if (result.ok()) {
                _currentBitrate = bps;
            } else {
                log(std::move(result), false);
            }
        }
    }
}

void VideoEncoder::setCurrentFramerate(double framerateFps)
{
    if (_maxFramerate) {
        const auto framerate = static_cast<uint32_t>(bound<double>(0., framerateFps + 0.5, _maxFramerate));
        if (framerate != _currentFramerate) {
            auto result = setEncoderFrameRate(framerate);
            if (result.ok()) {
                _currentFramerate = framerate;
            } else {
                log(std::move(result), false);
            }
        }
    }
}

VideoEncoder::BitrateAdjuster::BitrateAdjuster(float minAdjustedBitratePct, float maxAdjustedBitratePct)
    : _minAdjustedBitratePct(minAdjustedBitratePct)
    , _maxAdjustedBitratePct(maxAdjustedBitratePct)
{
    reset();
}

void VideoEncoder::BitrateAdjuster::reset()
{
    auto impl = std::make_unique<webrtc::BitrateAdjuster>(_minAdjustedBitratePct, _maxAdjustedBitratePct);
    impl->SetTargetBitrateBps(targetBitrate());
    _impl(std::move(impl));
    _hasPendingBitrateUpdates = false;
}

bool VideoEncoder::BitrateAdjuster::setTargetBitrate(uint32_t bitrateBps)
{
    if (exchangeVal(bitrateBps, _targetBitrateBps)) {
        LOCK_READ_SAFE_OBJ(_impl);
        if (const auto& impl = _impl.constRef()) {
            impl->SetTargetBitrateBps(bitrateBps);
            return true;
        }
    }
    return false;
}

uint32_t VideoEncoder::BitrateAdjuster::adjustedBitrate() const
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->GetAdjustedBitrateBps();
    }
    return targetBitrate();
}

std::optional<uint32_t> VideoEncoder::BitrateAdjuster::estimatedBitrate()
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->GetEstimatedBitrateBps();
    }
    return std::nullopt;
}

void VideoEncoder::BitrateAdjuster::update(size_t frameSize)
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        impl->Update(frameSize);
        _hasPendingBitrateUpdates = true;
    }
}

std::shared_ptr<webrtc::BitrateAdjuster> VideoEncoder::BitrateAdjuster::createImpl() const
{
    auto impl = std::make_shared<webrtc::BitrateAdjuster>(_minAdjustedBitratePct, _maxAdjustedBitratePct);
    impl->SetTargetBitrateBps(targetBitrate());
    return impl;
}

} // namespace LiveKitCpp

namespace
{
    
inline unsigned long maxSampleRateH264(webrtc::H264Level level)
{
    switch (level) {
        case webrtc::H264Level::kLevel3:
            return 10368000;
        case webrtc::H264Level::kLevel3_1:
            return 27648000;
        case webrtc::H264Level::kLevel3_2:
            return 55296000;
        case webrtc::H264Level::kLevel4:
        case webrtc::H264Level::kLevel4_1:
            return 62914560;
        case webrtc::H264Level::kLevel4_2:
            return 133693440;
        case webrtc::H264Level::kLevel5:
            return 150994944;
        case webrtc::H264Level::kLevel5_1:
            return 251658240;
        case webrtc::H264Level::kLevel5_2:
            return 530841600;
        case webrtc::H264Level::kLevel1:
        case webrtc::H264Level::kLevel1_b:
        case webrtc::H264Level::kLevel1_1:
        case webrtc::H264Level::kLevel1_2:
        case webrtc::H264Level::kLevel1_3:
        case webrtc::H264Level::kLevel2:
        case webrtc::H264Level::kLevel2_1:
        case webrtc::H264Level::kLevel2_2:
        default:
            // Zero means auto rate setting.
            break;
    }
    return 0; // zero means auto rate setting
}
    
float maxAllowedFrameRateH264(webrtc::H264Level level, uint16_t width,
                              uint16_t height, float maxFrameRate)
{
    if (width && height) {
        if (const auto maxSampleRate = maxSampleRateH264(level)) {
            const uint32_t alignedWidth = (((width + 15) >> 4) << 4);
            const uint32_t alignedHeight = (((height + 15) >> 4) << 4);
            return std::min<float>(maxFrameRate, (1.f * maxSampleRate) / (alignedWidth * alignedHeight));
        }
        return maxFrameRate;
    }
    return 0.f;
}
    
}
