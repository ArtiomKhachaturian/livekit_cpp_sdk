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
#include "VTDecoder.h"
#include "CFMemoryPool.h"
#include "RtcUtils.h"
#include "VideoFrameBufferPoolSource.h"
#include "Utils.h"


namespace
{

using namespace LiveKitCpp;

inline void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, NSString* value) {
    if (dict && value) {
        dict[toNSString(key)] = value;
    }
}

inline void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, NSNumber* value) {
    if (dict && value) {
        dict[toNSString(key)] = value;
    }
}

inline void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, Boolean value) {
    if (dict) {
        dict[toNSString(key)] = @(value);
    }
}

inline void setDictionaryValue(NSMutableDictionary* dict, CFStringRef key, CFStringRef value) {
    setDictionaryValue(dict, key, toNSString(value));
}

// https://developer.apple.com/documentation/avfoundation/avassettrack/1386694-formatdescriptions?language=objc
NSString* cmVideoCodecTypeToString(CMVideoCodecType code);
CFStringRef primaries(webrtc::ColorSpace::PrimaryID primaryId);
CFStringRef transfer(webrtc::ColorSpace::TransferID transferId);
CFStringRef matrix(webrtc::ColorSpace::MatrixID matrixId);

}

namespace LiveKitCpp
{

VTDecoder::VTDecoder(const webrtc::SdpVideoFormat& format,
                     const std::shared_ptr<CFMemoryPool>& memoryPool,
                     OSType outputPixelFormat)
    : VideoDecoder(format)
    , _outputPixelFormat(outputPixelFormat)
    , _memoryPool(memoryPool ? memoryPool : CFMemoryPool::create())
    , _framesPool(VideoFrameBufferPoolSource::create())
{
}

VTDecoder::~VTDecoder()
{
    VTDecoder::destroySession();
}

bool VTDecoder::hardwareAccelerated() const
{
    if (_session) {
        return _session.hardwareAccelerated();
    }
    return VideoDecoder::hardwareAccelerated();
}

bool VTDecoder::Configure(const Settings& settings)
{
    if (VideoDecoder::Configure(settings)) {
        _numberOfCores = std::max(0, settings.number_of_cores());
        if (const auto imageFormat = createVideoFormat(settings.max_render_resolution())) {
            return logError(createSession(imageFormat, true)).ok();
        }
        return true; // no wait of acceptable video format, maybe in VTDecoder::Decode
    }
    return false;
}

int32_t VTDecoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs)
{
    if (!hasDecodeCompleteCallback()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (_session) {
        auto status = _session.lastOutputStatus();
        if (!status) {
            logError(std::move(status));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    const auto encodeBuffer = inputImage.GetEncodedData();
    if (!encodeBuffer) {
        logWarning(COMPLETION_STATUS(kVTVideoDecoderUnsupportedDataFormatErr));
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
        const auto imageFormat = createVideoFormat(inputImage);
        if (!imageFormat) {
            logError(COMPLETION_STATUS(kVTVideoDecoderUnsupportedDataFormatErr));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        // check if the video format has changed, and reinitialize decoder if needed
        if (!_session || !CMFormatDescriptionEqual(imageFormat, _session.format())) {
            const bool realtime = webrtc::VideoContentType::UNSPECIFIED == inputImage.content_type_;
            const auto sessionStatus = logError(createSession(imageFormat, realtime)); // retain
            if (!sessionStatus) {
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
        }
        else {
            CFRelease(imageFormat);
        }
    }
    const auto sampleBuffer = createSampleBuffer(inputImage, _session.format());
    if (!sampleBuffer) {
        logWarning(COMPLETION_STATUS(kCMBlockBufferEmptyBBufErr));
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    VTDecodeInfoFlags infoFlags;
    auto status = _session.decompress(sampleBuffer, inputImage, &infoFlags);
    if (!status) {
        logError(std::move(status));
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (testFlag<kVTDecodeInfo_FrameDropped>(infoFlags)) {
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

CMVideoFormatDescriptionRef VTDecoder::createVideoFormat(const webrtc::RenderResolution& resolution,
                                                         const webrtc::ColorSpace* colorSpace) const
{
    if (resolution.Valid()) {
        return createVideoFormat(resolution.Width(), resolution.Height(), colorSpace);
    }
    return nullptr;
}

CMVideoFormatDescriptionRef VTDecoder::createVideoFormat(uint32_t /*encodedWidth*/,
                                                         uint32_t /*encodedHeight*/,
                                                         const webrtc::ColorSpace* colorSpace) const
{
    if (const auto codecType = toVTCodecType(type())) {
        @autoreleasepool {
            NSMutableDictionary* extensions = [NSMutableDictionary new];
            setDictionaryValue(extensions, kCMFormatDescriptionExtension_FormatName, cmVideoCodecTypeToString(codecType.value()));
            // YCbCr without alpha uses 24. See
            // http://developer.apple.com/qa/qa2001/qa1183.html
            setDictionaryValue(extensions, kCMFormatDescriptionExtension_Depth, @24);
            if (colorSpace) {
                setDictionaryValue(extensions, kCMFormatDescriptionExtension_ColorPrimaries, primaries(colorSpace->primaries()));
                setDictionaryValue(extensions, kCMFormatDescriptionExtension_TransferFunction, transfer(colorSpace->transfer()));
                switch (colorSpace->transfer()) {
                    case webrtc::ColorSpace::TransferID::kGAMMA22:
                        setDictionaryValue(extensions, kCMFormatDescriptionExtension_GammaLevel, @2.2);
                        break;
                    case webrtc::ColorSpace::TransferID::kGAMMA28:
                        setDictionaryValue(extensions, kCMFormatDescriptionExtension_GammaLevel, @2.8);
                        break;
                    default:
                        break;
                }
                setDictionaryValue(extensions, kCMFormatDescriptionExtension_YCbCrMatrix, matrix(colorSpace->matrix()));
                // Set full range flag.
                setDictionaryValue(extensions, kCMFormatDescriptionExtension_FullRangeVideo,
                                   webrtc::ColorSpace::RangeID::kFull == colorSpace->range());
            }
        }
    }
    return nullptr;
}

CMVideoFormatDescriptionRef VTDecoder::createVideoFormat(const webrtc::EncodedImage& inputImage) const
{
    return createVideoFormat(inputImage._encodedWidth, inputImage._encodedHeight, inputImage.ColorSpace());
}

void VTDecoder::destroySession()
{
    if (_session) {
        logWarning(_session.waitForAsynchronousFrames());
        _session = {};
    }
    VideoDecoder::destroySession();
}

CompletionStatus VTDecoder::createSession(CFAutoRelease<CMVideoFormatDescriptionRef> format, bool realtime)
{
    destroySession();
    if (const auto vtCodec = toVTCodecType(type())) {
        auto session = VTDecoderSession::create(vtCodec.value(),
                                                std::move(format),
                                                realtime,
                                                _outputPixelFormat,
                                                _numberOfCores,
                                                this,
                                                VideoFrameBufferPool{_framesPool});
        if (session) {
            _session = session.moveValue();
            if (_session) {
                const auto poolSize = bufferPoolSize();
                if (poolSize > 0) {
                    if (_framesPool) {
                        _framesPool->resize(poolSize);
                    }
                    logWarning(_session.setOutputPoolRequestedMinimumBufferCount(poolSize));
                }
            }
        }
        return session.moveStatus();
    }
    return COMPLETION_STATUS(kVTCouldNotFindVideoDecoderErr);
}

void VTDecoder::onDecodedImage(CMTime timestamp, CMTime duration,
                               VTDecodeInfoFlags infoFlags,
                               uint32_t encodedImageTimestamp,
                               webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                               std::optional<uint8_t> qp,
                               std::optional<webrtc::ColorSpace> encodedImageColorspace)
{
    if (hasDecodeCompleteCallback() && !testFlag<kVTDecodeInfo_FrameDropped>(infoFlags) && buffer) {
        if (auto frame = createVideoFrame(buffer,
                                          webrtc::VideoRotation::kVideoRotation_0,
                                          cmTimeToMicro(timestamp),
                                          0U,
                                          encodedImageColorspace)) {
            frame->set_rtp_timestamp(encodedImageTimestamp);
            sendDecodedImage(frame.value(), cmTimeToMilli(duration), std::move(qp));
        }
        else {
            logWarning(COMPLETION_STATUS(kVTVideoDecoderMalfunctionErr));
        }
    }
}

void VTDecoder::onError(CompletionStatus error, bool fatal)
{
    if (fatal) {
        logError(std::move(error));
    }
    else {
        logWarning(std::move(error));
    }
}

} // namespace LiveKitCpp

namespace
{

// https://developer.apple.com/documentation/avfoundation/avassettrack/1386694-formatdescriptions?language=objc
NSString* cmVideoCodecTypeToString(CMVideoCodecType code)
{
    NSString* result = [NSString stringWithFormat:@"%c%c%c%c",
                        (code >> 24) & 0xff, (code >> 16) & 0xff,
                        (code >> 8) & 0xff, code & 0xff];
    NSCharacterSet* characterSet = [NSCharacterSet whitespaceCharacterSet];
    return [result stringByTrimmingCharactersInSet:characterSet];
}

CFStringRef primaries(webrtc::ColorSpace::PrimaryID primaryId)
{
    switch (primaryId) {
        case webrtc::ColorSpace::PrimaryID::kBT709:
        case webrtc::ColorSpace::PrimaryID::kUnspecified:  // Assume BT.709.
            return kCMFormatDescriptionColorPrimaries_ITU_R_709_2;
        case webrtc::ColorSpace::PrimaryID::kBT2020:
            return kCMFormatDescriptionColorPrimaries_ITU_R_2020;
        case webrtc::ColorSpace::PrimaryID::kSMPTE170M:
        case webrtc::ColorSpace::PrimaryID::kSMPTE240M:
            return kCMFormatDescriptionColorPrimaries_SMPTE_C;
        case webrtc::ColorSpace::PrimaryID::kBT470BG:
            return kCMFormatDescriptionColorPrimaries_EBU_3213;
        case webrtc::ColorSpace::PrimaryID::kSMPTEST431:
            return kCMFormatDescriptionColorPrimaries_DCI_P3;
        /*case webrtc::ColorSpace::PrimaryID::kSMPTEST432:
            return kCMFormatDescriptionColorPrimaries_P3_D65;*/
        default:
            RTC_LOG(LS_ERROR) << "Unsupported primary id: " << primaryId;
            break;
    }
    return nil;
}

CFStringRef transfer(webrtc::ColorSpace::TransferID transferId)
{
    switch (transferId) {
        case webrtc::ColorSpace::TransferID::kLINEAR:
            if (@available(macos 10.14, *)) {
                return kCMFormatDescriptionTransferFunction_Linear;
            }
            break;
        case webrtc::ColorSpace::TransferID::kGAMMA22:
        case webrtc::ColorSpace::TransferID::kGAMMA28:
            return kCMFormatDescriptionTransferFunction_UseGamma;

        case webrtc::ColorSpace::TransferID::kIEC61966_2_1:
            if (@available(macos 10.13, *)) {
                return kCVImageBufferTransferFunction_sRGB;
            }
            break;
        case webrtc::ColorSpace::TransferID::kSMPTE170M:
        case webrtc::ColorSpace::TransferID::kBT709:
        case webrtc::ColorSpace::TransferID::kUnspecified:  // Assume BT.709.
            return kCMFormatDescriptionTransferFunction_ITU_R_709_2;
        case webrtc::ColorSpace::TransferID::kBT2020_10:
        case webrtc::ColorSpace::TransferID::kBT2020_12:
            return kCMFormatDescriptionTransferFunction_ITU_R_2020;
        case webrtc::ColorSpace::TransferID::kSMPTEST2084:
            if (@available(macos 10.13, *)) {
                return kCMFormatDescriptionTransferFunction_SMPTE_ST_2084_PQ;
            }
            break;
        case webrtc::ColorSpace::TransferID::kARIB_STD_B67:
            if (@available(macos 10.13, *)) {
                return kCMFormatDescriptionTransferFunction_ITU_R_2100_HLG;
            }
            break;
        case webrtc::ColorSpace::TransferID::kSMPTE240M:
            return kCMFormatDescriptionTransferFunction_SMPTE_240M_1995;
        case webrtc::ColorSpace::TransferID::kSMPTEST428:
            if (@available(macos 10.12, *)) {
                return kCMFormatDescriptionTransferFunction_SMPTE_ST_428_1;
            }
            break;
        default:
            break;
    }
    return nil;
}

CFStringRef matrix(webrtc::ColorSpace::MatrixID matrixId)
{
    switch (matrixId) {
        case webrtc::ColorSpace::MatrixID::kBT709:
        case webrtc::ColorSpace::MatrixID::kUnspecified:  // Assume BT.709.
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_709_2;
        case webrtc::ColorSpace::MatrixID::kBT2020_NCL:
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_2020;
        case webrtc::ColorSpace::MatrixID::kFCC:
        case webrtc::ColorSpace::MatrixID::kSMPTE170M:
        case webrtc::ColorSpace::MatrixID::kBT470BG:
            // The FCC-based coefficients don't exactly match BT.601, but they're close enough
            return kCMFormatDescriptionYCbCrMatrix_ITU_R_601_4;
        case webrtc::ColorSpace::MatrixID::kSMPTE240M:
            return kCMFormatDescriptionYCbCrMatrix_SMPTE_240M_1995;
        default:
            break;
    }
    return nil;
}

}
