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
#include "MFPipeline.h"
#include "MFCommon.h"
#include "MFTransformConfigurator.h"
#include "RtcUtils.h"
#include "Utils.h"
#include <Mferror.h>
#include <Wmcodecdsp.h>
#include <mfapi.h>
#include <mfidl.h>

namespace LiveKitCpp 
{

MFPipeline::MFPipeline(bool encoder, bool hardwareAccellerated,
                       DWORD inputStreamID, DWORD outputStreamID,
                       std::string friendlyName,
                       MFInitializer mftInitializer,
                       const CComPtr<IMFTransform>& transform,
                       const CComPtr<IMFAttributes>& attributes,
                       const CComPtr<IMFMediaEventGenerator>& eventGenerator)
    : _encoder(encoder)
    , _hardwareAccellerated(hardwareAccellerated)
    , _inputStreamID(inputStreamID)
    , _outputStreamID(outputStreamID)
    , _friendlyName(std::move(friendlyName))
    , _transform(transform)
    , _attributes(attributes)
    , _eventGenerator(eventGenerator)
{
}

webrtc::RTCErrorOr<MFPipeline> MFPipeline::create(bool video,
                                                  bool encoder, bool sync,
                                                  bool software,
                                                  bool allowTranscoders,
                                                  const GUID& compressedType,
                                                  const GUID& uncompressedType,
                                                  MFTransformConfigurator* configurator)
{
    MFInitializer mftInitializer(true);
    auto status = toRtcError(mftInitializer);
    if (!status.ok()) {
        return status;
    }
    UINT32 desiredFlags = MFT_ENUM_FLAG_ALL, actualFlags = 0U;
    if (software) {
        desiredFlags &= ~MFT_ENUM_FLAG_HARDWARE;
    }
    if (sync) {
        desiredFlags &= ~MFT_ENUM_FLAG_ASYNCMFT;
    }
    else {
        desiredFlags &= ~MFT_ENUM_FLAG_SYNCMFT;
    }
    if (!allowTranscoders) {
        desiredFlags &= ~MFT_ENUM_FLAG_TRANSCODE_ONLY;
    }
    DWORD inputStreamID, outputStreamID;
    std::string friendlyName;
    auto transform = createTransform(compressedType, video, encoder,
                                     desiredFlags, uncompressedType,
                                     &inputStreamID, &outputStreamID,
                                     &actualFlags, &friendlyName,
                                     configurator);
    if (!transform.ok()) {
        if (testFlag<MFT_ENUM_FLAG_HARDWARE>(desiredFlags)) {
            desiredFlags &= ~MFT_ENUM_FLAG_HARDWARE; // disable HWA
            transform = createTransform(compressedType, video, encoder,
                                        desiredFlags, uncompressedType,
                                        &inputStreamID, &outputStreamID,
                                        &actualFlags, &friendlyName,
                                        configurator);
        }
        if (!transform.ok()) {
            const auto& predefinedCodec = predefinedCodecType(encoder, compressedType);
            if (GUID_NULL != predefinedCodec) {
                transform = createPredefinedTransform(predefinedCodec, encoder,
                                                      configurator, inputStreamID,
                                                      outputStreamID, actualFlags,
                                                      friendlyName);
            }
        }
    }
    if (!transform.ok()) {
        return transform.MoveError();
    }
    if (!acceptFlags(desiredFlags, actualFlags)) {
        return toRtcError(MF_E_NOT_FOUND, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    CComPtr<IMFAttributes> attributes;
    status = toRtcError(transform.value()->GetAttributes(&attributes));
    if (!status.ok()) {
        return status;
    }
    CComPtr<IMFMediaEventGenerator> eventGenerator;
    if (::MFGetAttributeUINT32(attributes, MF_TRANSFORM_ASYNC, FALSE)) {
        status = toRtcError(transform.value()->QueryInterface(&eventGenerator));
        if (!status.ok()) {
            return status;
        }
        if (eventGenerator) {
            // unlock the transform for async use if get event generator
            status = toRtcError(attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE));
            if (!status.ok()) {
                return status;
            }
        }
    }
    const auto hardware = testFlag<MFT_ENUM_FLAG_HARDWARE>(actualFlags);
    return MFPipeline(encoder, hardware, inputStreamID, outputStreamID,
                      std::move(friendlyName), std::move(mftInitializer),
                      transform.MoveValue(), attributes, eventGenerator);
}

webrtc::RTCError MFPipeline::beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState)
{
    if (_eventGenerator) {
        return toRtcError(_eventGenerator->BeginGetEvent(callback, punkState));
    }
    return toRtcError(E_NOTIMPL);
}

webrtc::RTCError MFPipeline::endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent)
{
    if (_eventGenerator) {
        return toRtcError(_eventGenerator->EndGetEvent(result, outEvent));
    }
    return toRtcError(E_NOTIMPL);
}

webrtc::RTCErrorOr<CComPtr<IMFMediaEvent>> MFPipeline::asyncEvent(DWORD flags) const
{
    if (_eventGenerator) {
        CComPtr<IMFMediaEvent> event;
        auto hr = toRtcError(_eventGenerator->GetEvent(flags, &event));
        if (hr.ok()) {
            return event;
        }
        return hr;
    }
    return toRtcError(E_NOTIMPL);
}

webrtc::RTCErrorOr<CComPtr<IMFMediaType>> MFPipeline::mediaType(bool input) const
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    CComPtr<IMFMediaType> mediaType;
    webrtc::RTCError status;
    if (input) {
        status = toRtcError(_transform->GetInputCurrentType(_inputStreamID, &mediaType));
    }
    else {
        status = toRtcError(_transform->GetOutputCurrentType(_outputStreamID, &mediaType));
    }
    if (status.ok()) {
        return mediaType;
    }
    return status;
}

webrtc::RTCError MFPipeline::setMediaType(bool input, const CComPtr<IMFMediaType>& mediaType)
{
    if (!mediaType) {
        return toRtcError(E_POINTER, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    if (input) {
        return toRtcError(_transform->SetInputType(_inputStreamID, mediaType, 0L));
    }
    return toRtcError(_transform->SetOutputType(_outputStreamID, mediaType, 0L));
}

webrtc::RTCError MFPipeline::setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return setMediaType(!encoder(), mediaType);
}

webrtc::RTCError MFPipeline::setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return setMediaType(encoder(), mediaType);
}

webrtc::RTCError MFPipeline::selectMediaType(bool input, const GUID& subType)
{
    if (GUID_NULL == subType) {
        return toRtcError(E_INVALIDARG, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    GUID availableSubType;
    CComPtr<IMFMediaType> selected;
    HRESULT hr = S_OK;
    for (DWORD typeIndex = 0UL;; ++typeIndex) {
        CComPtr<IMFMediaType> mediaType;
        if (input) {
            hr = _transform->GetInputAvailableType(_inputStreamID, typeIndex, &mediaType);
        }
        else {
            hr = _transform->GetOutputAvailableType(_outputStreamID, typeIndex, &mediaType);
        }
        if (MF_E_NO_MORE_TYPES == hr || FAILED(hr)) {
            break;
        }
        hr = mediaType->GetGUID(MF_MT_SUBTYPE, &availableSubType);
        if (FAILED(hr)) {
            break;
        }
        if (subType == availableSubType) {
            selected = mediaType;
            break;
        }
    }
    if (selected) {
        return setMediaType(input, selected);
    }
    return toRtcError(hr);
}

webrtc::RTCError MFPipeline::setLowLatencyMode(bool set)
{
    if (set != _lowLatency) {
        auto hr = toRtcError(setUINT32Attr(CODECAPI_AVLowLatencyMode, set ? TRUE : FALSE));
        if (hr.ok()) {
            _lowLatency = set;
        }
        return hr;
    }
    return {};
}

webrtc::RTCError MFPipeline::setRealtimeContent(bool set)
{
    if (set != _realtime) {
        auto compressed = compressedMediaType();
        if (compressed.ok()) {
            auto hr = toRtcError(compressed.value()->SetUINT32(MF_MT_REALTIME_CONTENT,
                                                               set ? TRUE : FALSE));
            if (hr.ok()) {
                _realtime = set;
            }
            return hr;
        }
        return compressed.MoveError();
    }
    return {};
}

webrtc::RTCError MFPipeline::start()
{
    if (!_started) {
        if (!encoder()) {
            flush(); // ignore flush errors
        }
        auto hr = toRtcError(processMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING));
        if (hr.ok()) {
            hr = toRtcError(processMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM));
            _started = hr.ok();
        }
        return hr;
    }
    return {};
}

webrtc::RTCError MFPipeline::stop()
{
    if (_started) {
        auto hr = toRtcError(processMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM));
        if (hr.ok()) {
            hr = toRtcError(processMessage(MFT_MESSAGE_COMMAND_DRAIN));
            if (hr.ok()) {
                if (!encoder()) {
                    flush(); // ignore flush errors
                }
                _started = false;
            }
        }
        return hr;
    }
    return {};
}

webrtc::RTCError MFPipeline::flush() 
{ 
    return toRtcError(processMessage(MFT_MESSAGE_COMMAND_FLUSH));
}

webrtc::RTCError MFPipeline::drain() 
{ 
    return toRtcError(processMessage(MFT_MESSAGE_COMMAND_DRAIN));
}

webrtc::RTCErrorOr<DWORD> MFPipeline::compressedStatus() const
{
    return encoder() ? outputStatus() : inputStatus();
}

webrtc::RTCErrorOr<DWORD> MFPipeline::uncompressedStatus() const
{
    return encoder() ? inputStatus() : outputStatus();
}

webrtc::RTCErrorOr<DWORD> MFPipeline::inputStatus() const
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    DWORD status = 0UL;
    auto hr = toRtcError(_transform->GetInputStatus(_inputStreamID, &status));
    if (hr.ok()) {
        return status;
    }
    return hr;
}

webrtc::RTCErrorOr<DWORD> MFPipeline::outputStatus() const
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    DWORD status = 0UL;
    auto hr = toRtcError(_transform->GetOutputStatus(&status));
    if (hr.ok()) {
        return status;
    }
    return hr;
}

webrtc::RTCErrorOr<CComPtr<IMFSample>> MFPipeline::createSampleWitMemoryBuffer(bool input)
{
    DWORD maxLength = 0UL, aligment = 0UL;
    if (input) {
        auto si = inputStreamInfo();
        if (si.ok()) {
            maxLength = si.value().cbSize;
            aligment = si.value().cbAlignment;
        } else {
            return si.MoveError();
        }
    } else {
        auto si = outputStreamInfo();
        if (si.ok()) {
            maxLength = si.value().cbSize;
            aligment = si.value().cbAlignment;
        }
        else {
            return si.MoveError();
        }
    }
    return LiveKitCpp::createSampleWitMemoryBuffer(maxLength, aligment);
}

webrtc::RTCError MFPipeline::processInput(const CComPtr<IMFSample>& sample, DWORD flags)
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    return toRtcError(_transform->ProcessInput(_inputStreamID, sample, flags));
}

webrtc::RTCErrorOr<CComPtr<IMFSample>> MFPipeline::processOutput(const CComPtr<IMFSample>& sample,
                                                                 const CComPtr<IMFCollection>& events)
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    DWORD status = 0UL;
    // create output buffer description
    MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
    outputDataBuffer.dwStatus = 0UL;
    outputDataBuffer.dwStreamID = _outputStreamID;
    outputDataBuffer.pEvents = events;
    outputDataBuffer.pSample = sample;
    auto hr = toRtcError(_transform->ProcessOutput(0UL, 1UL, &outputDataBuffer, &status));
    if (outputDataBuffer.pEvents) { // got events from ProcessOuput, but discarding
        outputDataBuffer.pEvents->Release();
    }
    if (hr.ok()) {
        return CComPtr<IMFSample>(outputDataBuffer.pSample);
    }
    return hr;
}

webrtc::RTCErrorOr<MFT_INPUT_STREAM_INFO> MFPipeline::inputStreamInfo() const
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    MFT_INPUT_STREAM_INFO streamInfo;
    auto hr = toRtcError(_transform->GetInputStreamInfo(_inputStreamID, &streamInfo));
    if (hr.ok()) {
        return streamInfo;
    }
    return hr;
}

webrtc::RTCErrorOr<MFT_OUTPUT_STREAM_INFO> MFPipeline::outputStreamInfo() const
{
    if (!_transform) {
        return toRtcError(E_NOT_VALID_STATE, webrtc::RTCErrorType::INVALID_STATE);
    }
    MFT_OUTPUT_STREAM_INFO streamInfo;
    auto hr = toRtcError(_transform->GetOutputStreamInfo(_outputStreamID, &streamInfo));
    if (hr.ok()) {
        return streamInfo;
    }
    return hr;
}

HRESULT MFPipeline::setUINT32Attr(const GUID& attribute, UINT32 value)
{
    return _attributes->SetUINT32(attribute, value);
}

HRESULT MFPipeline::processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    return _transform->ProcessMessage(message, param);
}

const GUID& MFPipeline::predefinedCodecType(bool encoder, const GUID& compressedType)
{
    if (encoder) {
        if (MFVideoFormat_H264 == compressedType) {
            return CLSID_MSH264EncoderMFT;
        }
        if (MFAudioFormat_MP3 == compressedType) {
            return CLSID_MP3ACMCodecWrapper;
        }
    } else {
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

webrtc::RTCErrorOr<CComPtr<IMFTransform>> MFPipeline::
    createPredefinedTransform(const GUID& codecType, bool encoder,
                              MFTransformConfigurator* configurator,
                              DWORD& inputStreamID, DWORD& outputStreamID,
                              UINT32& actualFlags, std::string& friendlyName)
{
    if (GUID_NULL == codecType) {
        return toRtcError(E_INVALIDARG, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    CComPtr<IMFTransform> transform;
    auto hr = toRtcError(transform.CoCreateInstance(codecType, NULL, CLSCTX_INPROC_SERVER));
    if (!hr.ok()) {
        return hr;
    }
    if (configurator) {
        hr = toRtcError(configurator->configure(transform));
        if (!hr.ok()) {
            return hr;
        }
    }
    auto ids = transformStreamIDs(transform);
    if (!ids.ok()) {
        return ids.MoveError();
    }
    inputStreamID = ids.value().first;
    outputStreamID = ids.value().second;
    CComPtr<IMFAttributes> attributes;
    if (SUCCEEDED(transform->GetAttributes(&attributes))) {
        actualFlags = ::MFGetAttributeUINT32(attributes, MF_TRANSFORM_FLAGS_Attribute, 0U);
        if (0U == actualFlags) {
            // only sync
            actualFlags = MFT_ENUM_FLAG_SYNCMFT;
        }
        friendlyName = transformFriendlyName(attributes).MoveValue();
    }
    return transform;
}

} // namespace LiveKitCpp