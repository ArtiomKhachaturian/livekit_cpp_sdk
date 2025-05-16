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
#include "MFCommon.h"
#include "MFTransformConfigurator.h"
#include "Utils.h"
#include "RtcUtils.h"
#include <Mferror.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Wmcodecdsp.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

namespace 
{

using namespace LiveKitCpp;

inline const GUID& transformCategory(bool video, bool encoder)
{
    if (video) {
        return encoder ? MFT_CATEGORY_VIDEO_ENCODER : MFT_CATEGORY_VIDEO_DECODER;
    }
    return encoder ? MFT_CATEGORY_AUDIO_ENCODER : MFT_CATEGORY_AUDIO_DECODER;
}

inline const GUID& mediaCategory(bool video)
{
    return video ? MFMediaType_Video : MFMediaType_Audio;
}

template <unsigned flag>
inline constexpr bool flagIsEqual(unsigned flagsL, unsigned flagsR)
{
    return testFlag<flag>(flagsL) == testFlag<flag>(flagsR);
}

} // namespace

namespace LiveKitCpp 
{

CompletionStatusOrComPtr<IMFTransform> createTransform(const GUID& compressedType,
                                                       bool video,
                                                       bool encoder,
                                                       UINT32 desiredFlags,
                                                       const GUID& uncompressedType,
                                                       DWORD* inputStreamID, DWORD* outputStreamID,
                                                       UINT32* actualFlags,
                                                       std::string* friendlyName,
                                                       MFTransformConfigurator* configurator)
{
    if (GUID_NULL == compressedType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    MFT_REGISTER_TYPE_INFO compressed = { mediaCategory(video), compressedType };
    std::unique_ptr<MFT_REGISTER_TYPE_INFO> uncompressed;
    if (GUID_NULL != uncompressedType) {
        uncompressed = std::make_unique<MFT_REGISTER_TYPE_INFO>();
        uncompressed->guidMajorType = mediaCategory(video);
        uncompressed->guidSubtype = uncompressedType;
    }
    CComHeapPtr<IMFActivate*> activateRaw;
    desiredFlags |= MFT_ENUM_FLAG_SORTANDFILTER;
    UINT32 activateCount = 0U;
    auto status = COMPLETION_STATUS(::MFTEnumEx(transformCategory(video, encoder),
                                                desiredFlags,
                                                encoder ? uncompressed.get() : &compressed,
                                                encoder ? &compressed : uncompressed.get(),
                                                &activateRaw, &activateCount));
    if (!status) {
        return status;
    }
    if (0U == activateCount) {
        return COMPLETION_STATUS(MF_E_NOT_FOUND);
    }
    CComPtr<IMFTransform> selectedTransform;
    UINT32 testedCount = 0U;
    CompletionStatus hr;
    for (UINT32 i = 0U; i < activateCount; i++) {
        if (auto& activate = activateRaw[i]) {
            if (!selectedTransform) {
                const auto flags = ::MFGetAttributeUINT32(activate,
                                                          MF_TRANSFORM_FLAGS_Attribute, 
                                                          0U);
                if (!acceptFlags(desiredFlags, flags)) {
                    continue;
                }
                CComPtr<IMFTransform> transform;
                hr = COMPLETION_STATUS(activate->ActivateObject(IID_PPV_ARGS(&transform)));
                if (hr) {
                    if (configurator) {
                        hr = configurator->configure(transform);
                    }
                    if (hr) {
                        if (inputStreamID && outputStreamID) {
                            auto ids = transformStreamIDs(transform);
                            if (ids) {
                                *inputStreamID = ids->first;
                                *outputStreamID = ids->second;
                            }
                            else {
                                hr = ids.moveStatus();
                            }
                        }
                        if (hr.ok()) {
                            if (actualFlags) {
                                *actualFlags = flags;
                            }
                            if (friendlyName) {
                                *friendlyName = transformFriendlyName(activate).moveValue();
                            }
                        }
                    }
                    if (hr.ok()) {
                        selectedTransform = transform;
                    }
                    else {
                        transform.Release();
                    }
                }
            }
            activate->Release();
            activate = NULL;
        }
        if (hr.ok()) {
            break;
        }
    }
    if (selectedTransform) {
        return selectedTransform;
    }
    if (hr.ok() && 0U == testedCount) {
        hr = COMPLETION_STATUS(MF_E_NOT_FOUND);
    }
    return hr;
}

CompletionStatusOr<std::pair<DWORD, DWORD>> transformStreamIDs(IMFTransform* transform)
{
    if (!transform) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    DWORD numIns = 0UL, numOuts = 0UL;
    auto hr = COMPLETION_STATUS(transform->GetStreamCount(&numIns, &numOuts));
    if (!hr) {
        return hr;
    }
    if (!numIns || !numOuts) {
        return COMPLETION_STATUS(MF_E_ASF_UNSUPPORTED_STREAM_TYPE);
    }
    std::vector<DWORD> inIDs(numIns, 0UL), outIDs(numOuts, 0UL);
    hr = COMPLETION_STATUS(transform->GetStreamIDs(numIns, inIDs.data(), numOuts, outIDs.data()));
    if (hr) {
        return std::make_pair(inIDs.front(), outIDs.front());
    }
    return std::make_pair<DWORD, DWORD>(0U, 0U); // zero for both ID
}

CompletionStatusOr<std::string> transformFriendlyName(IMFAttributes* attributes)
{
    if (!attributes) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    UINT32 nameLength = 0U;
    LPWSTR wName = NULL;
    auto hr = COMPLETION_STATUS(attributes->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute,
                                                               &wName, &nameLength));
    if (hr) {
        std::string name;
        if (nameLength && wName) {
            name = fromWideChar(std::wstring_view(wName, nameLength));
        }
        ::CoTaskMemFree(wName);
        return name;
    }
    return hr;
}

CompletionStatusOr<std::string> transformFriendlyName(IMFTransform* transform)
{
    if (!transform) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    CComPtr<IMFAttributes> attributes;
    auto status = COMPLETION_STATUS(transform->GetAttributes(&attributes));
    if (!status) {
        return status;
    }
    return transformFriendlyName(attributes);
}

CompletionStatusOrComPtr<IMFMediaType> createMediaType(bool video, const GUID& subtype)
{
    if (GUID_NULL == subtype) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    CComPtr<IMFMediaType> mediaType;
    auto hr = COMPLETION_STATUS(::MFCreateMediaType(&mediaType));
    if (!hr) {
        return hr;
    }
    hr = COMPLETION_STATUS(mediaType->SetGUID(MF_MT_MAJOR_TYPE, mediaCategory(video)));
    if (!hr) {
        return hr;
    }
    hr = COMPLETION_STATUS(mediaType->SetGUID(MF_MT_SUBTYPE, subtype));
    if (!hr) {
        return hr;
    }
    return mediaType;
}

CompletionStatus setAllSamplesIndependent(IMFMediaType* mediaType, bool set)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS(mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, set ? TRUE : FALSE));
}

CompletionStatus setFrameSize(IMFMediaType* mediaType, UINT32 width, UINT32 height)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS(::MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, width, height));
}

CompletionStatus setFramerate(IMFMediaType* mediaType, UINT32 num, UINT32 denum)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS(::MFSetAttributeRatio(mediaType, MF_MT_FRAME_RATE, num, denum));
}

CompletionStatus setFramerate(IMFMediaType* mediaType, UINT32 frameRate)
{
    return setFramerate(mediaType, frameRate, 1U);
}

CompletionStatus setPixelAspectRatio(IMFMediaType* mediaType, UINT32 num, UINT32 denum)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS(::MFSetAttributeRatio(mediaType, MF_MT_PIXEL_ASPECT_RATIO, num, denum));
}

CompletionStatus setPixelAspectRatio1x1(IMFMediaType* mediaType)
{
    return setPixelAspectRatio(mediaType, 1U, 1U);
}

CompletionStatus setInterlaceMode(IMFMediaType* mediaType, MFVideoInterlaceMode mode)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    return COMPLETION_STATUS(mediaType->SetUINT32(MF_MT_INTERLACE_MODE, mode));
}

CompletionStatusOr<std::pair<UINT32, UINT32>> frameSize(IMFMediaType* mediaType)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    UINT32 width = 0U, height = 0U;
    auto hr = COMPLETION_STATUS(::MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height));
    if (hr) {
        return std::make_pair(width, height);
    }
    return hr;
}

CompletionStatusOr<UINT32> framerate(IMFMediaType* mediaType)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    UINT num = 0U, denum = 0U;
    auto hr = COMPLETION_STATUS(::MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &num, &denum));
    if (hr) {
        return static_cast<UINT32>(std::round((1.f * num) / denum));
    }
    return hr;
}

CompletionStatusOrComPtr<IMFSample> createSample(IMFMediaBuffer* attachedBuffer)
{
    CComPtr<IMFSample> sample;
    auto hr = COMPLETION_STATUS(::MFCreateSample(&sample));
    if (!hr) {
        return hr;
    }
    if (attachedBuffer) {
        hr = COMPLETION_STATUS(sample->AddBuffer(attachedBuffer));
        if (!hr) {
            return hr;
        }
    }
    return sample;
}

CompletionStatusOrComPtr<IMFSample> createSampleWitMemoryBuffer(DWORD maxLength, DWORD aligment)
{
    CComPtr<IMFMediaBuffer> buffer;
    CompletionStatus status;
    if (aligment) {
        status = COMPLETION_STATUS(::MFCreateAlignedMemoryBuffer(maxLength, aligment, &buffer));
    } else {
        status = COMPLETION_STATUS(::MFCreateMemoryBuffer(maxLength, &buffer));
    }
    if (buffer) {
        return createSample(buffer);
    }
    return status;
}

bool acceptFlags(DWORD desired, DWORD actual)
{
    if (flagIsEqual<MFT_ENUM_FLAG_SYNCMFT>(desired, actual)) {
        const bool hardwareTest = flagIsEqual<MFT_ENUM_FLAG_HARDWARE>(desired, actual);
        bool asyncTest = flagIsEqual<MFT_ENUM_FLAG_ASYNCMFT>(desired, actual);
        if (!asyncTest && hardwareTest) { // hardware transforms is always async
            // MFT_ENUM_FLAG_HARDWARE -> Enumerates V2 hardware async MFTs
            asyncTest = true;
        }
        return hardwareTest && asyncTest;
    }
    return false;
}

const GUID& predefinedCodecType(bool encoder, const GUID& compressedType)
{
    if (encoder) {
        if (MFVideoFormat_H264 == compressedType) {
            return CLSID_MSH264EncoderMFT;
        }
        if (MFAudioFormat_MP3 == compressedType) {
            return CLSID_MP3ACMCodecWrapper;
        }
    }
    else {
        if (MFVideoFormat_VP80 == compressedType || MFVideoFormat_VP90 == compressedType) {
            return CLSID_MSVPxDecoder;
        }
        if (MFVideoFormat_H264 == compressedType) {
            return CLSID_MSH264DecoderMFT;
        }
        if (MFVideoFormat_H265 == compressedType) {
            return CLSID_MSH265DecoderMFT;
        }
        if (MFAudioFormat_MP3 == compressedType) {
            return CLSID_CMP3DecMediaObject;
        }
        if (MFAudioFormat_AAC == compressedType) {
            return CLSID_MSAACDecMFT;
        }
        if (MFAudioFormat_MPEG == compressedType) {
            return CLSID_MSMPEGAudDecMFT;
        }
        if (MFAudioFormat_Opus == compressedType) {
            return CLSID_MSOpusDecoder;
        }
    }
    return GUID_NULL;
}

const GUID& compressedType(webrtc::VideoCodecType type)
{
    switch (type) {
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

} // namespace LiveKitCpp