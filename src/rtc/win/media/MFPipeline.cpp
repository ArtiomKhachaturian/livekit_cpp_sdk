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
                       CComPtr<IMFTransform> transform,
                       CComPtr<IMFAttributes> attributes,
                       CComPtr<IMFMediaEventGenerator> eventGenerator)
    : _encoder(encoder)
    , _hardwareAccellerated(hardwareAccellerated)
    , _inputStreamID(inputStreamID)
    , _outputStreamID(outputStreamID)
    , _friendlyName(std::move(friendlyName))
    , _transform(std::move(transform))
    , _attributes(std::move(attributes))
    , _eventGenerator(std::move(eventGenerator))
{
}

CompletionStatusOr<MFPipeline> MFPipeline::create(bool video,
                                                  bool encoder, bool sync,
                                                  bool software,
                                                  bool allowTranscoders,
                                                  const GUID& compressedType,
                                                  const GUID& uncompressedType,
                                                  MFTransformConfigurator* configurator)
{
    MFInitializer mftInitializer(true);
    auto status = COMPLETION_STATUS(mftInitializer);
    if (!status) {
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
    if (!transform) {
        if (testFlag<MFT_ENUM_FLAG_HARDWARE>(desiredFlags)) {
            desiredFlags &= ~MFT_ENUM_FLAG_HARDWARE; // disable HWA
            transform = createTransform(compressedType, video, encoder,
                                        desiredFlags, uncompressedType,
                                        &inputStreamID, &outputStreamID,
                                        &actualFlags, &friendlyName,
                                        configurator);
        }
        if (!transform) {
            const auto& predefinedCodec = predefinedCodecType(encoder, compressedType);
            if (GUID_NULL != predefinedCodec) {
                transform = createPredefinedTransform(predefinedCodec, encoder,
                                                      configurator, inputStreamID,
                                                      outputStreamID, actualFlags,
                                                      friendlyName);
            }
        }
    }
    if (!transform) {
        return transform.moveStatus();
    }
    if (!acceptFlags(desiredFlags, actualFlags)) {
        return COMPLETION_STATUS(MF_E_NOT_FOUND);
    }
    CComPtr<IMFAttributes> attributes;
    status = COMPLETION_STATUS(transform.value()->GetAttributes(&attributes));
    if (!status) {
        return status;
    }
    CComPtr<IMFMediaEventGenerator> eventGenerator;
    if (::MFGetAttributeUINT32(attributes, MF_TRANSFORM_ASYNC, FALSE)) {
        status = COMPLETION_STATUS(transform.value()->QueryInterface(&eventGenerator));
        if (!status) {
            return status;
        }
        if (eventGenerator) {
            // unlock the transform for async use if get event generator
            status = COMPLETION_STATUS(attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE));
            if (!status) {
                return status;
            }
        }
    }
    const auto hardware = testFlag<MFT_ENUM_FLAG_HARDWARE>(actualFlags);
    return MFPipeline(encoder, hardware, inputStreamID, outputStreamID,
                      std::move(friendlyName), std::move(mftInitializer),
                      transform.moveValue(), std::move(attributes), 
                      std::move(eventGenerator));
}

CompletionStatus MFPipeline::beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState)
{
    if (_eventGenerator) {
        return COMPLETION_STATUS(_eventGenerator->BeginGetEvent(callback, punkState));
    }
    return COMPLETION_STATUS(E_NOTIMPL);
}

CompletionStatus MFPipeline::endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent)
{
    if (_eventGenerator) {
        return COMPLETION_STATUS(_eventGenerator->EndGetEvent(result, outEvent));
    }
    return COMPLETION_STATUS(E_NOTIMPL);
}

CompletionStatusOrComPtr<IMFMediaEvent> MFPipeline::asyncEvent(DWORD flags) const
{
    if (_eventGenerator) {
        CComPtr<IMFMediaEvent> event;
        auto hr = COMPLETION_STATUS(_eventGenerator->GetEvent(flags, &event));
        if (hr) {
            return event;
        }
        return hr;
    }
    return COMPLETION_STATUS(E_NOTIMPL);
}

CompletionStatusOrComPtr<IMFMediaType> MFPipeline::mediaType(bool input) const
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    CComPtr<IMFMediaType> mediaType;
    CompletionStatus status;
    if (input) {
        status = COMPLETION_STATUS(_transform->GetInputCurrentType(_inputStreamID, &mediaType));
    }
    else {
        status = COMPLETION_STATUS(_transform->GetOutputCurrentType(_outputStreamID, &mediaType));
    }
    if (status) {
        return mediaType;
    }
    return status;
}

CompletionStatus MFPipeline::setMediaType(bool input, const CComPtr<IMFMediaType>& mediaType)
{
    if (!mediaType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    if (input) {
        return COMPLETION_STATUS(_transform->SetInputType(_inputStreamID, mediaType, 0L));
    }
    return COMPLETION_STATUS(_transform->SetOutputType(_outputStreamID, mediaType, 0L));
}

CompletionStatus MFPipeline::setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return setMediaType(!encoder(), mediaType);
}

CompletionStatus MFPipeline::setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType)
{
    return setMediaType(encoder(), mediaType);
}

CompletionStatus MFPipeline::selectMediaType(bool input, const GUID& subType)
{
    if (GUID_NULL == subType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    GUID availableSubType;
    CComPtr<IMFMediaType> selected;
    CompletionStatus hr;
    for (DWORD typeIndex = 0UL;; ++typeIndex) {
        CComPtr<IMFMediaType> mediaType;
        if (input) {
            hr = COMPLETION_STATUS(_transform->GetInputAvailableType(_inputStreamID, typeIndex, &mediaType));
        }
        else {
            hr = COMPLETION_STATUS(_transform->GetOutputAvailableType(_outputStreamID, typeIndex, &mediaType));
        }
        if (MF_E_NO_MORE_TYPES == hr.code() || !hr) {
            break;
        }
        hr = COMPLETION_STATUS(mediaType->GetGUID(MF_MT_SUBTYPE, &availableSubType));
        if (!hr) {
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
    return hr;
}

CompletionStatus MFPipeline::setLowLatencyMode(bool set)
{
    if (set != _lowLatency) {
        auto hr = COMPLETION_STATUS(setUINT32Attr(CODECAPI_AVLowLatencyMode, set ? TRUE : FALSE));
        if (hr) {
            _lowLatency = set;
        }
        return hr;
    }
    return {};
}

CompletionStatus MFPipeline::setRealtimeContent(bool set)
{
    if (set != _realtime) {
        auto compressed = compressedMediaType();
        if (compressed) {
            auto hr = COMPLETION_STATUS(compressed.value()->SetUINT32(MF_MT_REALTIME_CONTENT,
                                                                       set ? TRUE : FALSE));
            if (hr) {
                _realtime = set;
            }
            return hr;
        }
        return compressed.moveStatus();
    }
    return {};
}

CompletionStatus MFPipeline::start()
{
    if (!_started) {
        if (!encoder()) {
            flush(); // ignore flush errors
        }
        auto hr = processMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING);
        if (hr) {
            hr = processMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM);
            _started = hr.ok();
        }
        return hr;
    }
    return {};
}

CompletionStatus MFPipeline::stop()
{
    if (_started) {
        auto hr = processMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM);
        if (hr) {
            hr = processMessage(MFT_MESSAGE_COMMAND_DRAIN);
            if (hr) {
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

CompletionStatus MFPipeline::flush() 
{ 
    return processMessage(MFT_MESSAGE_COMMAND_FLUSH);
}

CompletionStatus MFPipeline::drain() 
{ 
    return processMessage(MFT_MESSAGE_COMMAND_DRAIN);
}

CompletionStatusOr<DWORD> MFPipeline::compressedStatus() const
{
    return encoder() ? outputStatus() : inputStatus();
}

CompletionStatusOr<DWORD> MFPipeline::uncompressedStatus() const
{
    return encoder() ? inputStatus() : outputStatus();
}

CompletionStatusOr<DWORD> MFPipeline::inputStatus() const
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    DWORD status = 0UL;
    auto hr = COMPLETION_STATUS(_transform->GetInputStatus(_inputStreamID, &status));
    if (hr) {
        return status;
    }
    return hr;
}

CompletionStatusOr<DWORD> MFPipeline::outputStatus() const
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    DWORD status = 0UL;
    auto hr = COMPLETION_STATUS(_transform->GetOutputStatus(&status));
    if (hr) {
        return status;
    }
    return hr;
}

CompletionStatusOrComPtr<IMFSample> MFPipeline::createSampleWitMemoryBuffer(bool input) const
{
    DWORD maxLength = 0UL, aligment = 0UL;
    if (input) {
        auto si = inputStreamInfo();
        if (si) {
            maxLength = si.value().cbSize;
            aligment = si.value().cbAlignment;
        } else {
            return si.moveStatus();
        }
    } else {
        auto si = outputStreamInfo();
        if (si) {
            maxLength = si.value().cbSize;
            aligment = si.value().cbAlignment;
        }
        else {
            return si.moveStatus();
        }
    }
    return LiveKitCpp::createSampleWitMemoryBuffer(maxLength, aligment);
}

CompletionStatus MFPipeline::processInput(const CComPtr<IMFSample>& sample, DWORD flags)
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    return COMPLETION_STATUS(_transform->ProcessInput(_inputStreamID, sample, flags));
}

CompletionStatusOrComPtr<IMFSample> MFPipeline::processOutput(const CComPtr<IMFSample>& sample,
                                                              const CComPtr<IMFCollection>& events)
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    DWORD status = 0UL;
    // create output buffer description
    MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
    outputDataBuffer.dwStatus = 0UL;
    outputDataBuffer.dwStreamID = _outputStreamID;
    outputDataBuffer.pEvents = events;
    outputDataBuffer.pSample = sample;
    auto hr = COMPLETION_STATUS(_transform->ProcessOutput(0UL, 1UL, &outputDataBuffer, &status));
    if (outputDataBuffer.pEvents) { // got events from ProcessOuput, but discarding
        outputDataBuffer.pEvents->Release();
    }
    if (hr) {
        return CComPtr<IMFSample>(outputDataBuffer.pSample);
    }
    return hr;
}

CompletionStatusOr<MFT_INPUT_STREAM_INFO> MFPipeline::inputStreamInfo() const
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    MFT_INPUT_STREAM_INFO streamInfo;
    auto hr = COMPLETION_STATUS(_transform->GetInputStreamInfo(_inputStreamID, &streamInfo));
    if (hr) {
        return streamInfo;
    }
    return hr;
}

CompletionStatusOr<MFT_OUTPUT_STREAM_INFO> MFPipeline::outputStreamInfo() const
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    MFT_OUTPUT_STREAM_INFO streamInfo;
    auto hr = COMPLETION_STATUS(_transform->GetOutputStreamInfo(_outputStreamID, &streamInfo));
    if (hr) {
        return streamInfo;
    }
    return hr;
}

HRESULT MFPipeline::setUINT32Attr(const GUID& attribute, UINT32 value)
{
    return _attributes ? _attributes->SetUINT32(attribute, value) : E_NOT_VALID_STATE;
}

CompletionStatus MFPipeline::processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    if (!_transform) {
        return COMPLETION_STATUS_INVALID_STATE;
    }
    return COMPLETION_STATUS(_transform->ProcessMessage(message, param));
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

CompletionStatusOrComPtr<IMFTransform> MFPipeline::
    createPredefinedTransform(const GUID& codecType, bool encoder,
                              MFTransformConfigurator* configurator,
                              DWORD& inputStreamID, DWORD& outputStreamID,
                              UINT32& actualFlags, std::string& friendlyName)
{
    if (GUID_NULL == codecType) {
        return COMPLETION_STATUS_INVALID_ARG;
    }
    CComPtr<IMFTransform> transform;
    auto hr = COMPLETION_STATUS(transform.CoCreateInstance(codecType, NULL, CLSCTX_INPROC_SERVER));
    if (!hr) {
        return hr;
    }
    if (configurator) {
        hr = configurator->configure(transform);
        if (!hr) {
            return hr;
        }
    }
    auto ids = transformStreamIDs(transform);
    if (!ids) {
        return ids.moveStatus();
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
        friendlyName = transformFriendlyName(attributes).moveValue();
    }
    return transform;
}

} // namespace LiveKitCpp