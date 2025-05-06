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
#include "MFInitializer.h"
#include <atlbase.h> //CComPtr support
#include <api/rtc_error.h>
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
    static webrtc::RTCErrorOr<MFPipeline> create(bool video,
                                                 bool encoder,
                                                 bool sync,
                                                 bool software,
                                                 bool allowTranscoders,
                                                 const GUID& compressedType,
                                                 const GUID& uncompressedType = GUID_NULL,
                                                 MFTransformConfigurator* configurator = nullptr);
    explicit operator bool() const { return valid(); }
    MFPipeline& operator = (const MFPipeline&) = delete;
    MFPipeline& operator = (MFPipeline&&) = default;
    bool valid() const { return NULL != _transform; }
    bool encoder() const { return _encoder; }
    bool hardwareAccellerated() const { return _hardwareAccellerated; }
    bool sync() const { return NULL == _eventGenerator; }
    bool started() const { return _started; }
    const CComPtr<IMFAttributes>& attributes() const { return _attributes; }
    const std::string& friendlyName() const { return _friendlyName; }
    webrtc::RTCError beginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState = NULL);
    webrtc::RTCError endGetEvent(IMFAsyncResult* result, IMFMediaEvent** outEvent);
    webrtc::RTCErrorOr<CComPtr<IMFMediaEvent>> asyncEvent(DWORD flags = MF_EVENT_FLAG_NO_WAIT) const;
    webrtc::RTCErrorOr<CComPtr<IMFMediaType>> mediaType(bool input) const;
    webrtc::RTCErrorOr<CComPtr<IMFMediaType>> compressedMediaType() const { return mediaType(!encoder()); }
    webrtc::RTCErrorOr<CComPtr<IMFMediaType>> uncompressedMediaType() const { return mediaType(encoder()); }
    webrtc::RTCError setMediaType(bool input, const CComPtr<IMFMediaType>& mediaType);
    webrtc::RTCError setCompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    webrtc::RTCError setUncompressedMediaType(const CComPtr<IMFMediaType>& mediaType);
    webrtc::RTCError selectMediaType(bool input, const GUID& subType);
    webrtc::RTCError setLowLatencyMode(bool set);
    webrtc::RTCError setRealtimeContent(bool set);
    webrtc::RTCError start();
    webrtc::RTCError stop();
    webrtc::RTCError flush();
    webrtc::RTCError drain();
    webrtc::RTCErrorOr<DWORD> compressedStatus() const;
    webrtc::RTCErrorOr<DWORD> uncompressedStatus() const;
    webrtc::RTCErrorOr<DWORD> inputStatus() const;
    webrtc::RTCErrorOr<DWORD> outputStatus() const;
    webrtc::RTCErrorOr<CComPtr<IMFSample>> createSampleWitMemoryBuffer(bool input);
    webrtc::RTCError processInput(const CComPtr<IMFSample>& sample, DWORD flags = 0UL);
    webrtc::RTCErrorOr<CComPtr<IMFSample>> processOutput(const CComPtr<IMFSample>& sample,
                                                         const CComPtr<IMFCollection>& events = {});
    webrtc::RTCErrorOr<MFT_INPUT_STREAM_INFO> inputStreamInfo() const;
    webrtc::RTCErrorOr<MFT_OUTPUT_STREAM_INFO> outputStreamInfo() const;
    HRESULT setUINT32Attr(const GUID& attribute, UINT32 value);
    HRESULT processMessage(MFT_MESSAGE_TYPE message, ULONG_PTR param = NULL);
private:
    MFPipeline(bool encoder, bool hardwareAccellerated,
               DWORD inputStreamID, DWORD outputStreamID,
               std::string friendlyName,
               MFInitializer mftInitializer,
               const CComPtr<IMFTransform>& transform,
               const CComPtr<IMFAttributes>& attributes,
               const CComPtr<IMFMediaEventGenerator>& eventGenerator = {});
    static const GUID& predefinedCodecType(bool encoder, const GUID& compressedType);
    static webrtc::RTCErrorOr<CComPtr<IMFTransform>> 
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