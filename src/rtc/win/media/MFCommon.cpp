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

webrtc::RTCErrorOr<CComPtr<IMFTransform>> createTransform(const GUID& compressedType,
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
        return toRtcError(E_INVALIDARG, webrtc::RTCErrorType::INVALID_PARAMETER);
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
    const auto status = ::MFTEnumEx(transformCategory(video, encoder),
                                    desiredFlags,
                                    encoder ? uncompressed.get() : &compressed,
                                    encoder ? &compressed : uncompressed.get(),
                                    &activateRaw, &activateCount);
    if (FAILED(status)) {
        return toRtcError(status);
    }
    if (0U == activateCount) {
        return toRtcError(MF_E_NOT_FOUND);
    }
    CComPtr<IMFTransform> selectedTransform;
    UINT32 testedCount = 0U;
    webrtc::RTCError hr;
    for (UINT32 i = 0U; i < activateCount; i++) {
        if (auto& activate = activateRaw[i]) {
            if (!selectedTransform) {
                const auto flags = ::MFGetAttributeUINT32(activate,
                    MF_TRANSFORM_FLAGS_Attribute, 0U);
                if (!acceptFlags(desiredFlags, flags)) {
                    continue;
                }
                CComPtr<IMFTransform> transform;
                hr = toRtcError(activate->ActivateObject(IID_PPV_ARGS(&transform)));
                if (hr.ok()) {
                    if (configurator) {
                        hr = toRtcError(configurator->configure(transform));
                    }
                    if (hr.ok()) {
                        if (inputStreamID && outputStreamID) {
                            auto ids = transformStreamIDs(transform);
                            if (ids.ok()) {
                                *inputStreamID = ids.value().first;
                                *outputStreamID = ids.value().second;
                            }
                            else {
                                hr = ids.MoveError();
                            }
                        }
                        if (hr.ok()) {
                            if (actualFlags) {
                                *actualFlags = flags;
                            }
                            if (friendlyName) {
                                *friendlyName = transformFriendlyName(activate).MoveValue();
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
        hr = toRtcError(MF_E_NOT_FOUND);
    }
    return hr;
}

webrtc::RTCErrorOr<std::pair<DWORD, DWORD>> transformStreamIDs(const CComPtr<IMFTransform>& transform)
{
    if (!transform) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    DWORD numIns = 0UL, numOuts = 0UL;
    auto hr = toRtcError(transform->GetStreamCount(&numIns, &numOuts));
    if (!hr.ok()) {
        return hr;
    }
    if (!numIns || !numOuts) {
        return toRtcError(MF_E_ASF_UNSUPPORTED_STREAM_TYPE);
    }
    std::vector<DWORD> inIDs(numIns, 0UL), outIDs(numOuts, 0UL);
    hr = toRtcError(transform->GetStreamIDs(numIns, inIDs.data(), numOuts, outIDs.data()));
    if (hr.ok()) {
        return std::make_pair(inIDs.front(), outIDs.front());
    }
    return std::make_pair<DWORD, DWORD>(0U, 0U); // zero for both ID
}

webrtc::RTCErrorOr<std::string> transformFriendlyName(IMFAttributes* attributes)
{
    if (!attributes) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    UINT32 nameLength = 0U;
    LPWSTR wName = NULL;
    auto hr = toRtcError(attributes->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute,
                                                        &wName, &nameLength));
    if (hr.ok()) {
        std::string name;
        if (nameLength && wName) {
            name = fromWideChar(std::wstring_view(wName, nameLength));
        }
        ::CoTaskMemFree(wName);
        return name;
    }
    return hr;
}

webrtc::RTCErrorOr<CComPtr<IMFMediaType>> createMediaType(bool video, const GUID& subtype)
{
    if (GUID_NULL == subtype) {
        return toRtcError(E_INVALIDARG, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    CComPtr<IMFMediaType> mediaType;
    auto hr = toRtcError(::MFCreateMediaType(&mediaType));
    if (!hr.ok()) {
        return hr;
    }
    hr = toRtcError(mediaType->SetGUID(MF_MT_MAJOR_TYPE, mediaCategory(video)));
    if (!hr.ok()) {
        return hr;
    }
    hr = toRtcError(mediaType->SetGUID(MF_MT_SUBTYPE, subtype));
    if (!hr.ok()) {
        return hr;
    }
    return mediaType;
}

webrtc::RTCError setAllSamplesIndependent(const CComPtr<IMFMediaType>& mediaType, bool set)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    return toRtcError(mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, set ? TRUE : FALSE));
}

webrtc::RTCError setFrameSize(const CComPtr<IMFMediaType>& mediaType, UINT32 width, UINT32 height)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    return toRtcError(::MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, width, height));
}

webrtc::RTCError setFramerate(const CComPtr<IMFMediaType>& mediaType, UINT32 num, UINT32 denum)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    return toRtcError(::MFSetAttributeRatio(mediaType, MF_MT_FRAME_RATE, num, denum));
}

webrtc::RTCError setFramerate(const CComPtr<IMFMediaType>& mediaType, UINT32 frameRate)
{
    return setFramerate(mediaType, frameRate, 1U);
}

webrtc::RTCError setPixelAspectRatio(const CComPtr<IMFMediaType>& mediaType, UINT32 num, UINT32 denum)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    return toRtcError(::MFSetAttributeRatio(mediaType, MF_MT_PIXEL_ASPECT_RATIO, num, denum));
}

webrtc::RTCError setPixelAspectRatio1x1(const CComPtr<IMFMediaType>& mediaType)
{
    return setPixelAspectRatio(mediaType, 1U, 1U);
}

webrtc::RTCErrorOr<std::pair<UINT32, UINT32>> frameSize(const CComPtr<IMFMediaType>& mediaType)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    UINT32 width = 0U, height = 0U;
    auto hr = toRtcError(::MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height));
    if (hr.ok()) {
        return std::make_pair(width, height);
    }
    return hr;
}

webrtc::RTCErrorOr<UINT32> framerate(const CComPtr<IMFMediaType>& mediaType)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    UINT num = 0U, denum = 0U;
    auto hr = toRtcError(::MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &num, &denum));
    if (hr.ok()) {
        return static_cast<UINT32>(std::round((1.f * num) / denum));
    }
    return hr;
}

webrtc::RTCErrorOr<CComPtr<IMFSample>> createSample(const CComPtr<IMFMediaBuffer>& attachedBuffer)
{
    CComPtr<IMFSample> sample;
    auto hr = toRtcError(::MFCreateSample(&sample));
    if (!hr.ok()) {
        return hr;
    }
    if (attachedBuffer) {
        hr = toRtcError(sample->AddBuffer(attachedBuffer));
        if (!hr.ok()) {
            return hr;
        }
    }
    return sample;
}

webrtc::RTCErrorOr<CComPtr<IMFSample>> createSampleWitMemoryBuffer(DWORD maxLength, DWORD aligment)
{
    CComPtr<IMFMediaBuffer> buffer;
    webrtc::RTCError status;
    if (aligment) {
        status = toRtcError(::MFCreateAlignedMemoryBuffer(maxLength, aligment, &buffer));
    } else {
        status = toRtcError(::MFCreateMemoryBuffer(maxLength, &buffer));
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

} // namespace LiveKitCpp