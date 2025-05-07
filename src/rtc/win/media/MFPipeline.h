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
#pragma once
#include "CompletionStatusOr.h"
#include "MFInitializer.h"
#include <atlbase.h> //CComPtr support
#include <codecapi.h>
#include <mftransform.h>

namespace LiveKitCpp 
{

class MFTransformConfigurator;

class MFPipeline
{
public:
    MFPipeline() = default;
    MFPipeline(MFPipeline&&) = default;
    MFPipeline(const MFPipeline&) = delete;
    ~MFPipeline() { stop(); }
    static CompletionStatusOr<MFPipeline> create(bool video,
                                                 bool encoder,
                                                 bool sync,
                                                 bool hardwareAccellerated,
                                                 bool allowTranscoders,
                                                 const GUID& compressedType,
                                                 const GUID& uncompressedType = GUID_NULL,
                                                 MFTransformConfigurator* configurator = nullptr);
    explicit operator bool() const { return valid(); }
    MFPipeline& operator = (const MFPipeline&) = delete;
    MFPipeline& operator = (MFPipeline&&) = default;
    bool valid() const { return NULL != _transform; }
    bool encoder() const noexcept { return _encoder; }
    bool hardwareAccellerated() const noexcept { return _hardwareAccellerated; }
    bool sync() const { return NULL == _eventGenerator; }
    bool started() const noexcept { return _started; }
    const auto& attributes() const noexcept { return _attributes; }
    const auto& friendlyName() const noexcept { return _friendlyName; }
    CompletionStatus beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState = NULL);
    CompletionStatus endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent);
    CompletionStatusOrComPtr<IMFMediaEvent> asyncEvent(DWORD flags = MF_EVENT_FLAG_NO_WAIT) const;
    CompletionStatusOrComPtr<IMFMediaType> mediaType(bool input) const;
    CompletionStatusOrComPtr<IMFMediaType> compressedMediaType() const { return mediaType(!encoder()); }
    CompletionStatusOrComPtr<IMFMediaType> uncompressedMediaType() const { return mediaType(encoder()); }
    CompletionStatus setMediaType(bool input, const CComPtr<IMFMediaType>& mediaType);
    CompletionStatus setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    CompletionStatus setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    CompletionStatus selectMediaType(bool input, const GUID& subType);
    CompletionStatus setLowLatencyMode(bool set);
    CompletionStatus setRealtimeContent(bool set);
    CompletionStatus start();
    CompletionStatus stop();
    CompletionStatus flush();
    CompletionStatus drain();
    CompletionStatusOr<DWORD> compressedStatus() const;
    CompletionStatusOr<DWORD> uncompressedStatus() const;
    CompletionStatusOr<DWORD> inputStatus() const;
    CompletionStatusOr<DWORD> outputStatus() const;
    CompletionStatusOrComPtr<IMFSample> createSampleWitMemoryBuffer(bool input) const;
    CompletionStatus processInput(const CComPtr<IMFSample>& sample, DWORD flags = 0UL);
    CompletionStatusOrComPtr<IMFSample> processOutput(const CComPtr<IMFSample>& sample,
                                                      const CComPtr<IMFCollection>& events = {});
    CompletionStatusOr<MFT_INPUT_STREAM_INFO> inputStreamInfo() const;
    CompletionStatusOr<MFT_OUTPUT_STREAM_INFO> outputStreamInfo() const;
    HRESULT setUINT32Attr(const GUID& attribute, UINT32 value);
    CompletionStatus processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param = NULL);
private:
    MFPipeline(bool encoder, bool hardwareAccellerated,
               DWORD inputStreamID, DWORD outputStreamID,
               std::string friendlyName,
               MFInitializer mftInitializer,
               CComPtr<IMFTransform> transform,
               CComPtr<IMFAttributes> attributes,
               CComPtr<IMFMediaEventGenerator> eventGenerator = {});
    static const GUID& predefinedCodecType(bool encoder, const GUID& compressedType);
    static CompletionStatusOrComPtr<IMFTransform>
        createPredefinedTransform(const GUID& codecType, bool encoder,
                                  MFTransformConfigurator* configurator, // opt
                                  DWORD& inputStreamID, DWORD& outputStreamID,
                                  UINT32& actualFlags, std::string& friendlyName);
private:
    bool _encoder = false;
    bool _hardwareAccellerated = false;
    DWORD _inputStreamID = {};
    DWORD _outputStreamID = {};
    std::string _friendlyName;
    MFInitializer _mftInitializer;
    CComPtr<IMFTransform> _transform;
    CComPtr<IMFAttributes> _attributes;
    CComPtr<IMFMediaEventGenerator> _eventGenerator;
    bool _started = false;
    bool _lowLatency = false;
    bool _realtime = false;
};

} // namespace LiveKitCpp