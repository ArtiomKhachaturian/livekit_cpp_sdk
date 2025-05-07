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
#include "MFH264Encoder.h"
#include "EncodedImageBuffer.h"
#include "MFMediaBufferLocker.h"
#include "MFVideoEncoderPipeline.h"
#include "MemoryBlock.h"
#include "H264Utils.h"
#include "RtcUtils.h"
#include "Utils.h"
#include <Mferror.h>
#include <common_video/h264/h264_common.h>

namespace 
{

class SimpleMemoryBlock : public LiveKitCpp::MemoryBlock
{
public:
    SimpleMemoryBlock();
    ~SimpleMemoryBlock() final;

protected:
    uint8_t* reallocate(uint8_t* data, size_t newSize) final;
};

} // namespace

namespace LiveKitCpp 
{

class MFH264Encoder::NaluHeader
{
public:
    NaluHeader(std::vector<BYTE> data, std::vector<webrtc::H264::NaluIndex> indices);
    const BYTE* data() const { return _data.data(); }
    size_t dataSize() const { return _data.size(); }
    const std::vector<webrtc::H264::NaluIndex>& indices() const { return _indices; }
private:
    const std::vector<BYTE> _data;
    const std::vector<webrtc::H264::NaluIndex> _indices;
};

class MFH264Encoder::H264EncoderBuffer : public webrtc::EncodedImageBufferInterface
{
public:
    H264EncoderBuffer(MFMediaBufferLocker locker);
    // impl. of webrtc::EncodedImageBufferInterface
    const uint8_t* data() const final { return _locker.dataBuffer(); }
    uint8_t* data() final { return _locker.dataBuffer(); }
    size_t size() const final { return _locker.currentLen(); }

private:
    const MFMediaBufferLocker _locker;
};

MFH264Encoder::MFH264Encoder(const webrtc::SdpVideoFormat& format,
                             std::optional<webrtc::H264ProfileLevelId> profileLevelId,
                             webrtc::H264PacketizationMode packetizationMode)
    : MFVideoEncoder(format, H264Utils::createCodecInfo(packetizationMode))
    , _profileLevelId(std::move(profileLevelId))
{
}

MFH264Encoder::~MFH264Encoder()
{
}

std::unique_ptr<webrtc::VideoEncoder> MFH264Encoder::create(const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoEncoder> encoder;
    if (H264Utils::formatMatched(format)) {
        const auto status = encoderStatus(format);
        if (CodecStatus::NotSupported != status) {
            auto profileLevelId = webrtc::ParseSdpForH264ProfileLevelId(format.parameters);
            const auto packetizationMode = H264Utils::packetizationMode(format);
            encoder.reset(new MFH264Encoder(format, std::move(profileLevelId), packetizationMode));
        }
    }
    return encoder;
}

int32_t MFH264Encoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t result = WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    if (codecSettings) {
        _h264params = codecSettings->H264();
        if (_profileLevelId.has_value()) {
            auto h264settings = H264Utils::map(*codecSettings, _profileLevelId->level);
            result = MFVideoEncoder::InitEncode(&h264settings, encoderSettings);
        } else {
            result = MFVideoEncoder::InitEncode(codecSettings, encoderSettings);
        }
    }
    return result;
}

CompletionStatus MFH264Encoder::processMpegHeaderData(std::vector<BYTE> naluHeaderData)
{
    auto indices = webrtc::H264::FindNaluIndices(rtc::MakeArrayView(naluHeaderData.data(), naluHeaderData.size()));
    if (indices.size() == H264BitstreamParser::naluParametersCount()) {
        _naluHeader = std::make_unique<NaluHeader>(std::move(naluHeaderData), std::move(indices));
    } else {
        _naluHeader.reset();
    }
    if (_naluHeader) {
        return {};
    }
    return COMPLETION_STATUS(E_FAIL);
}

CompletionStatusOr<MFVideoEncoderPipeline> MFH264Encoder::createPipeline(UINT32 width, UINT32 height)
{
    auto pipeline = MFVideoEncoder::createPipeline(width, height);
    if (pipeline) {
        if (_profileLevelId.has_value()) {
            bool allowCabac = false;
            if (const auto profile = toMFH264(_profileLevelId->profile)) {
                logError(pipeline->setH264Profile(profile.value()), false);
                // https://en.wikipedia.org/wiki/Context-adaptive_binary_arithmetic_coding
                // only supported in the Main and higher profiles (but not the extended profile)
                allowCabac = H264Utils::cabacIsSupported(_profileLevelId->profile) &&
                             eAVEncH264VProfile::eAVEncH264VProfile_Extended != profile.value();
            }
            if (const auto level = toMFH264(_profileLevelId->level)) {
                logError(pipeline->setH264Level(level.value()), false);
            }
            // CABAC generally gives better compression at the expense of higher computational overhead,
            // for avoiding of bitrate overshoot
            logError(pipeline->setH264CABACEnable(allowCabac), false);
        }
        if (_h264params.keyFrameInterval > 0) {
            logError(pipeline->setMPVGOPSize(_h264params.keyFrameInterval), false);
        }
        logError(pipeline->setTemporalLayerCount(_h264params.numberOfTemporalLayers), false);
    }
    return pipeline;
}

CompletionStatusOr<webrtc::EncodedImage> MFH264Encoder::
    createEncodedImage(bool keyFrame, const CComPtr<IMFMediaBuffer>& data,
                       const std::optional<UINT32>& encodedQp)
{
    if (data) {
        //if (_naluHeader) {
        MFMediaBufferLocker locker(data, false);
        if (locker.ok()) {
            rtc::scoped_refptr<webrtc::EncodedImageBufferInterface> encodedBuffer;
            /*if (keyFrame) {
                    auto block = std::make_unique<SimpleMemoryBlock>();
                    if (H264BitstreamParser::addNaluForKeyFrame(block.get(), _naluHeader->data(),
                                                                _naluHeader->indices())) {
                        block->appendData(locker.dataBuffer(), locker.currentLen());
                        encodedBuffer = EncodedImageBuffer::create(std::move(block));
                    }
                } else {
                    encodedBuffer = rtc::make_ref_counted<H264EncoderBuffer>(std::move(locker));
                }*/
            encodedBuffer = rtc::make_ref_counted<H264EncoderBuffer>(std::move(locker));
            if (encodedBuffer) {
                webrtc::EncodedImage image;
                if (encodedQp.has_value()) {
                    image.qp_ = encodedQp.value();
                } else {
                    _h264BitstreamParser.parseForSliceQp(encodedBuffer);
                    image.qp_ = _h264BitstreamParser.lastSliceQp();
                }
                image.SetEncodedData(std::move(encodedBuffer));
                return image;
            }
        }
        return COMPLETION_STATUS(locker.status());
    }
    return COMPLETION_STATUS_INVALID_ARG;
}

std::optional<eAVEncH264VProfile> MFH264Encoder::toMFH264(webrtc::H264Profile profile)
{
    switch (profile) {
        case webrtc::H264Profile::kProfileConstrainedBaseline:
            return eAVEncH264VProfile::eAVEncH264VProfile_ConstrainedBase;
        case webrtc::H264Profile::kProfileBaseline:
            return eAVEncH264VProfile::eAVEncH264VProfile_Base;
        case webrtc::H264Profile::kProfileMain:
            return eAVEncH264VProfile::eAVEncH264VProfile_Main;
        case webrtc::H264Profile::kProfileConstrainedHigh:
            return eAVEncH264VProfile::eAVEncH264VProfile_UCConstrainedHigh;
        case webrtc::H264Profile::kProfileHigh:
            return eAVEncH264VProfile::eAVEncH264VProfile_High;
        case webrtc::H264Profile::kProfilePredictiveHigh444:
            return eAVEncH264VProfile::eAVEncH264VProfile_444;
        default:
            assert(false);
            break;
    }
    return std::nullopt;
}

std::optional<eAVEncH264VLevel> MFH264Encoder::toMFH264(webrtc::H264Level level)
{
    switch (level) {
        case webrtc::H264Level::kLevel1_b:
            return eAVEncH264VLevel::eAVEncH264VLevel1_b;
        case webrtc::H264Level::kLevel1:
            return eAVEncH264VLevel::eAVEncH264VLevel1;
        case webrtc::H264Level::kLevel1_1:
            return eAVEncH264VLevel::eAVEncH264VLevel1_1;
        case webrtc::H264Level::kLevel1_2:
            return eAVEncH264VLevel::eAVEncH264VLevel1_2;
        case webrtc::H264Level::kLevel1_3:
            return eAVEncH264VLevel::eAVEncH264VLevel1_3;
        case webrtc::H264Level::kLevel2:
            return eAVEncH264VLevel::eAVEncH264VLevel2;
        case webrtc::H264Level::kLevel2_1:
            return eAVEncH264VLevel::eAVEncH264VLevel2_1;
        case webrtc::H264Level::kLevel2_2:
            return eAVEncH264VLevel::eAVEncH264VLevel2_2;
        case webrtc::H264Level::kLevel3:
            return eAVEncH264VLevel::eAVEncH264VLevel3;
        case webrtc::H264Level::kLevel3_1:
            return eAVEncH264VLevel::eAVEncH264VLevel3_1;
        case webrtc::H264Level::kLevel3_2:
            return eAVEncH264VLevel::eAVEncH264VLevel2_2;
        case webrtc::H264Level::kLevel4:
            return eAVEncH264VLevel::eAVEncH264VLevel4;
        case webrtc::H264Level::kLevel4_1:
            return eAVEncH264VLevel::eAVEncH264VLevel4_1;
        case webrtc::H264Level::kLevel4_2:
            return eAVEncH264VLevel::eAVEncH264VLevel4_2;
        case webrtc::H264Level::kLevel5:
            return eAVEncH264VLevel::eAVEncH264VLevel5;
        case webrtc::H264Level::kLevel5_1:
            return eAVEncH264VLevel::eAVEncH264VLevel5_1;
        case webrtc::H264Level::kLevel5_2:
            return eAVEncH264VLevel::eAVEncH264VLevel5_2;
        default:
            assert(false);
            break;
    }
    return std::nullopt;
}

MFH264Encoder::NaluHeader::NaluHeader(std::vector<BYTE> data, std::vector<webrtc::H264::NaluIndex> indices)
    : _data(std::move(data))
    , _indices(std::move(indices))
{
}


MFH264Encoder::H264EncoderBuffer::H264EncoderBuffer(MFMediaBufferLocker locker)
    : _locker(std::move(locker))
{
}

} // namespace LiveKitCpp

namespace {

SimpleMemoryBlock::SimpleMemoryBlock()
{
    ensureCapacity(LiveKitCpp::H264BitstreamParser::annexBHeaderSize());
}

SimpleMemoryBlock::~SimpleMemoryBlock()
{
    std::free(data());
}

uint8_t* SimpleMemoryBlock::reallocate(uint8_t* data, size_t newSize)
{
    return reinterpret_cast<uint8_t*>(std::realloc(data, newSize));
}

} // namespace