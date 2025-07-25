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
#pragma once // MFMediaSink.h
#include "MFTStreamSink.h"
#include <Mferror.h>
#include <mfidl.h>
#include <windows.foundation.h>
#include <windows.media.h>
#include <windows.media.mediaproperties.h>
#include <wrl.h>

namespace LiveKitCpp 
{

class MFMediaSink : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
    ABI::Windows::Media::IMediaExtension, 
    Microsoft::WRL::FtmBase, 
    IMFMediaSink, 
    IMFClockStateSink>
{
    InspectableClass(L"MFMediaSink", BaseTrust)
public:
    MFMediaSink() = default;
    HRESULT RuntimeClassInitialize() { return S_OK; }
    HRESULT registerEncodingCallback(MFTEncodingCallback* callback);
    // IMediaExtension
    HRESULT STDMETHODCALLTYPE SetProperties(ABI::Windows::Foundation::Collections::IPropertySet*) final { return S_OK; }
    // IMFMediaSink methods
    HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD* characteristics) final;
    HRESULT STDMETHODCALLTYPE AddStreamSink(DWORD streamSinkIdentifier, IMFMediaType* mediaType, 
                                            IMFStreamSink** outStreamSink) final;
    HRESULT STDMETHODCALLTYPE RemoveStreamSink(DWORD streamSinkIdentifier) final;
    HRESULT STDMETHODCALLTYPE GetStreamSinkCount(DWORD* streamSinkCount) final;
    HRESULT STDMETHODCALLTYPE GetStreamSinkByIndex(DWORD index, IMFStreamSink** outStreamSink) final;
    HRESULT STDMETHODCALLTYPE GetStreamSinkById(DWORD identifier, IMFStreamSink** outStreamSink) final;
    HRESULT STDMETHODCALLTYPE SetPresentationClock(IMFPresentationClock* presentationClock) final;
    HRESULT STDMETHODCALLTYPE GetPresentationClock(IMFPresentationClock** outPresentationClock) final;
    HRESULT STDMETHODCALLTYPE Shutdown() final;
    // IMFClockStateSink methods
    HRESULT STDMETHODCALLTYPE OnClockStart(MFTIME hnsSystemTime, LONGLONG clockStartOffset) final;
    HRESULT STDMETHODCALLTYPE OnClockStop(MFTIME hnsSystemTime) final;
    HRESULT STDMETHODCALLTYPE OnClockPause(MFTIME /*hnsSystemTime*/) final { return MF_E_INVALID_STATE_TRANSITION; }
    HRESULT STDMETHODCALLTYPE OnClockRestart(MFTIME /*hnsSystemTime*/) final { return MF_E_INVALID_STATE_TRANSITION; }
    HRESULT STDMETHODCALLTYPE OnClockSetRate(MFTIME /*hnsSystemTime*/, float /*rate*/) final { return S_OK; }
private:
    // all below operations are not thread-safe
    HRESULT checkShutdown() const { return _shutdown ? MF_E_SHUTDOWN : S_OK; }
private:
    webrtc::Mutex _lock;
    bool _shutdown = false;
    Microsoft::WRL::ComPtr<IMFPresentationClock> _clock;
    Microsoft::WRL::ComPtr<MFTStreamSink> _outputStream;
};

} // namespace LiveKitCpp 