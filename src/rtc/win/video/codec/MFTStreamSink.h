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
#pragma once // MFTStreamSink.h
#include "MFAsyncCallback.h"
#include "MFTEncodingCallback.h"
#include "Listener.h"
#include "rtc_base/synchronization/mutex.h"
#include <mfidl.h>
#include <Mferror.h>
#include <wrl.h>

namespace LiveKitCpp 
{

class MFTStreamSink : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
        IMFStreamSink,
        IMFMediaEventGenerator,
        IMFMediaTypeHandler>
{
    InspectableClass(L"MFTStreamSink", BaseTrust)
    template <typename T> using WrlComPtr = Microsoft::WRL::ComPtr<T>;
    enum class State : uint16_t;
    enum class Operation : uint16_t;
    class IAsyncStreamSinkOperation;
    class AsyncStreamSinkOperation;
public:
    MFTStreamSink();
    ~MFTStreamSink() override;
    HRESULT RuntimeClassInitialize(DWORD identifier, IMFMediaSink* parent);
    HRESULT start(MFTIME start);
    HRESULT stop();
    void shutdown();
    void registerEncodingCallback(MFTEncodingCallback* callback) { _callback = callback; }
    // IMFMediaEventGenerator
    HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState) final;
    HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** eventOut) final;
    HRESULT STDMETHODCALLTYPE GetEvent(DWORD flags, IMFMediaEvent** eventOut) final;
    HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType met, REFGUID guidExtendedType,
                                         HRESULT status, PROPVARIANT const* pvValue) final;
    // IMFStreamSink
    HRESULT STDMETHODCALLTYPE GetMediaSink(IMFMediaSink** outMediaSink) final;
    HRESULT STDMETHODCALLTYPE GetIdentifier(DWORD* outIdentifier) final;
    HRESULT STDMETHODCALLTYPE GetMediaTypeHandler(IMFMediaTypeHandler** outHandler) final;
    HRESULT STDMETHODCALLTYPE ProcessSample(IMFSample* sample) final;
    HRESULT STDMETHODCALLTYPE PlaceMarker(MFSTREAMSINK_MARKER_TYPE markerType,
                                          PROPVARIANT const* varMarkerValue,
                                          PROPVARIANT const* varContextValue) final;
    HRESULT STDMETHODCALLTYPE Flush() final;
    // IMFMediaTypeHandler
    HRESULT STDMETHODCALLTYPE IsMediaTypeSupported(IMFMediaType* mediaType, 
                                                   IMFMediaType** outMediaType) final;
    HRESULT STDMETHODCALLTYPE GetMediaTypeCount(DWORD* outTypeCount) final;
    HRESULT STDMETHODCALLTYPE GetMediaTypeByIndex(DWORD dwIndex, IMFMediaType** outMediaType) final;
    HRESULT STDMETHODCALLTYPE SetCurrentMediaType(IMFMediaType* mediaType) final;
    HRESULT STDMETHODCALLTYPE GetCurrentMediaType(IMFMediaType** outMediaType) final;
    HRESULT STDMETHODCALLTYPE GetMajorType(GUID* outGuidMajorType) final;
private:
    HRESULT onDispatchWorkItem(IMFAsyncResult* asyncResult);
    // all below operations are not thread-safe
    HRESULT checkShutdown() const { return _shutdown ? MF_E_SHUTDOWN : S_OK;  }
    HRESULT validateOperation(Operation op) const;
    HRESULT queueAsyncOperation(Operation op, const PROPVARIANT* propVariant = nullptr);
    HRESULT mediaTypeSupported(IMFMediaType* mediaType) const;
    HRESULT processFormatChange();
    CComPtr<IMFSample> processSamplesFromQueue();
    void dropSamplesFromQueue();
private:
    static const BOOL _validStateMatrix[4][5];
    MFAsyncCallback<MFTStreamSink> _workQueueCb;
    Bricks::Listener<MFTEncodingCallback*> _callback;
    webrtc::Mutex _lock;
    DWORD _identifier = (DWORD)-1;
    State _state;
    bool _shutdown = false;
    GUID _currentSubtype = {};
    DWORD _workQueueId = 0U;
    WrlComPtr<IMFMediaSink> _sink;
    WrlComPtr<IMFMediaEventQueue> _eventQueue;
    WrlComPtr<IMFMediaType> _currentType;
    std::list<CComPtr<IMFSample>> _sampleQueue;
};

} // namespace LiveKitCpp 