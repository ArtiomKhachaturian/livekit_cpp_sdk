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
#include "MFTStreamSink.h"
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <type_traits>

namespace LiveKitCpp 
{

enum class MFTStreamSink::State : uint16_t
{
    TypeNotSet = 0,    // No media type is set
    Ready,             // Media type is set, Start has never been called.
    Started,
    Stopped
};

enum class MFTStreamSink::Operation : uint16_t
{
    SetMediaType = 0,
    Start,
    Stop,
    ProcessSample,
    PlaceMarker
};

class DECLSPEC_UUID("4b35435f-44ae-44a0-9ba0-b84f9f4a9c19") 
    MFTStreamSink::IAsyncStreamSinkOperation : public IUnknown 
{
public:
    STDMETHOD(GetOp)(Operation* op) PURE;
    STDMETHOD(GetPropVariant)(PROPVARIANT * propVariant) PURE;
};

class DECLSPEC_UUID("0c89c2e1-79bb-4ad7-a34f-cc006225f8e1")
    MFTStreamSink::AsyncStreamSinkOperation : public Microsoft::WRL::RuntimeClass<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
        IAsyncStreamSinkOperation> {
    InspectableClass(L"MFTStreamSink::AsyncStreamSinkOperation", BaseTrust)
public:
    HRESULT RuntimeClassInitialize(Operation op, const PROPVARIANT* propVariant);
    ~AsyncStreamSinkOperation() override = default;
    HRESULT STDMETHODCALLTYPE GetOp(Operation* op) final;
    HRESULT STDMETHODCALLTYPE GetPropVariant(PROPVARIANT * propVariant) final;
private:
    PROPVARIANT _propVariant;
    Operation _op;
};

const BOOL MFTStreamSink::_validStateMatrix[4][5]{
    // States:    Operations:
    //            SetType Start Stop Sample
    /* NotSet */  {TRUE, FALSE, FALSE, FALSE},
    /* Ready */   {TRUE,  TRUE,  TRUE, FALSE},
    /* Start */   {TRUE,  TRUE,  TRUE,  TRUE},
    /* Stop */    {TRUE,  TRUE,  TRUE, FALSE}
};

MFTStreamSink::MFTStreamSink()
    : _workQueueCb(this, &MFTStreamSink::onDispatchWorkItem)
    , _state(State::TypeNotSet)
{
}

MFTStreamSink::~MFTStreamSink()
{
}

HRESULT MFTStreamSink::RuntimeClassInitialize(DWORD identifier, IMFMediaSink* parent)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = MFCreateEventQueue(&_eventQueue);
    // Allocate a new work queue for async operations.
    if (SUCCEEDED(hr)) {
        hr = MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD, &_workQueueId);
    }
    if (SUCCEEDED(hr)) {
        _sink = parent;
        _identifier = identifier;
    }
    return hr;
}

HRESULT MFTStreamSink::start(MFTIME start)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = validateOperation(Operation::Start);
    if (SUCCEEDED(hr)) {
        _state = State::Started;
        hr = queueAsyncOperation(Operation::Start);
    }
    return hr;
}

HRESULT MFTStreamSink::stop()
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = validateOperation(Operation::Stop);
    if (SUCCEEDED(hr)) {
        _state = State::Stopped;
        hr = queueAsyncOperation(Operation::Stop);
    }
    return hr;
}

void MFTStreamSink::shutdown()
{
    const webrtc::MutexLock locker(&_lock);
    if (!_shutdown) {
        if (_eventQueue) {
            _eventQueue->Shutdown();
        }
        MFUnlockWorkQueue(_workQueueId);

        _sampleQueue.clear();

        _sink.Reset();
        _eventQueue.Reset();
        _currentType.Reset();

        _callback.reset();;

        _shutdown = true;
    }
}

HRESULT MFTStreamSink::BeginGetEvent(IMFAsyncCallback* callback, IUnknown* punkState)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_eventQueue) {
            hr = MF_E_NOT_INITIALIZED;
        }
        else {
            hr = _eventQueue->BeginGetEvent(callback, punkState);
        }
    }
    return hr;
}

HRESULT MFTStreamSink::EndGetEvent(IMFAsyncResult* result, IMFMediaEvent** eventOut)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_eventQueue) {
            hr = MF_E_NOT_INITIALIZED;
        }
        else {
            hr = _eventQueue->EndGetEvent(result, eventOut);
        }
    }
    return hr;
}

HRESULT MFTStreamSink::GetEvent(DWORD flags, IMFMediaEvent** eventOut)
{
    // NOTE:
    // GetEvent can block indefinitely, so we don't hold the lock.
    // This requires some juggling with the event queue pointer.
    HRESULT hr = S_OK;
    WrlComPtr<IMFMediaEventQueue> eventQueue;
    {
        const webrtc::MutexLock locker(&_lock);
        hr = checkShutdown();
        if (SUCCEEDED(hr)) {
            eventQueue = _eventQueue;
            if (!eventQueue) {
                hr = MF_E_NOT_INITIALIZED;
            }
        }
    }
    if (eventQueue) {
        hr = eventQueue->GetEvent(flags, eventOut);
    }
    return hr;
}

HRESULT MFTStreamSink::QueueEvent(MediaEventType met, REFGUID guidExtendedType,
                                  HRESULT status, PROPVARIANT const* pvValue)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_eventQueue) {
            hr = MF_E_NOT_INITIALIZED;
        }
        else {
            hr = _eventQueue->QueueEventParamVar(met, guidExtendedType, status, pvValue);
        }
    }
    return hr;
}

HRESULT MFTStreamSink::GetMediaSink(IMFMediaSink** outMediaSink)
{
    if (!outMediaSink) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        *outMediaSink = _sink.Get();
        if (*outMediaSink) {
            (*outMediaSink)->AddRef();
        }
    }
    return hr;
}

HRESULT MFTStreamSink::GetIdentifier(DWORD* outIdentifier)
{
    if (!outIdentifier) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        *outIdentifier = _identifier;
    }
    return hr;
}

HRESULT MFTStreamSink::GetMediaTypeHandler(IMFMediaTypeHandler** outHandler)
{
    if (!outHandler) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        hr = QueryInterface(IID_IMFMediaTypeHandler, (void**)outHandler);
    }
    return hr;
}

HRESULT MFTStreamSink::ProcessSample(IMFSample* sample)
{
    if (!sample) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        hr = validateOperation(Operation::ProcessSample);
        if (SUCCEEDED(hr)) {
            _sampleQueue.push_back(sample);
            hr = queueAsyncOperation(Operation::ProcessSample);
        }
    }
    return hr;
}

HRESULT MFTStreamSink::PlaceMarker(MFSTREAMSINK_MARKER_TYPE markerType,
                                   PROPVARIANT const* /*varMarkerValue*/,
                                   PROPVARIANT const* varContextValue)
{
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        hr = queueAsyncOperation(Operation::PlaceMarker, varContextValue);
    }
    return hr;
}

HRESULT MFTStreamSink::Flush()
{
    const webrtc::MutexLock locker(&_lock);
    const HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        dropSamplesFromQueue();
    }
    return hr;
}

HRESULT MFTStreamSink::IsMediaTypeSupported(IMFMediaType* mediaType,
                                            IMFMediaType** outMediaType)
{
    if (!mediaType) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        hr = mediaTypeSupported(mediaType);
    }
    if (outMediaType) {
        *outMediaType = NULL;
    }
    return hr;
}

HRESULT MFTStreamSink::GetMediaTypeCount(DWORD* outTypeCount)
{
    if (!outTypeCount) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        *outTypeCount = 1U;
    }
    return S_OK;
}

HRESULT MFTStreamSink::GetMediaTypeByIndex(DWORD dwIndex, IMFMediaType** outMediaType)
{
    if (!outMediaType) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (dwIndex > 0U) {
            hr = MF_E_NO_MORE_TYPES;
        }
        else {
            *outMediaType = _currentType.Get();
            if (*outMediaType) {
                (*outMediaType)->AddRef();
            }
        }
    }
    return hr;
}

HRESULT MFTStreamSink::SetCurrentMediaType(IMFMediaType* mediaType)
{
    if (!mediaType) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        hr = validateOperation(Operation::SetMediaType);
        if (SUCCEEDED(hr)) {
            if (_state >= State::Ready) {
                hr = mediaTypeSupported(mediaType);
            }
            if (SUCCEEDED(hr)) {
                hr = MFCreateMediaType(_currentType.ReleaseAndGetAddressOf());
                if (SUCCEEDED(hr)) {
                    hr = mediaType->CopyAllItems(_currentType.Get());
                    if (SUCCEEDED(hr)) {
                        hr = _currentType->GetGUID(MF_MT_SUBTYPE, &_currentSubtype);
                        if (SUCCEEDED(hr)) {
                            if (_state < State::Ready) {
                                _state = State::Ready;
                            }
                            else if (_state > State::Ready) {
                                if (SUCCEEDED(hr)) {
                                    processFormatChange(); // return HRESULT
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT MFTStreamSink::GetCurrentMediaType(IMFMediaType** outMediaType)
{
    if (!outMediaType) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_currentType) {
            hr = MF_E_NOT_INITIALIZED;
        }
        else {
            *outMediaType = _currentType.Get();
            (*outMediaType)->AddRef();
        }
    }
    return hr;
}

HRESULT MFTStreamSink::GetMajorType(GUID* outGuidMajorType)
{
    if (!outGuidMajorType) {
        return E_INVALIDARG;
    }
    const webrtc::MutexLock locker(&_lock);
    HRESULT hr = checkShutdown();
    if (SUCCEEDED(hr)) {
        if (!_currentType) {
            hr = MF_E_NOT_INITIALIZED;
        }
        else {
            hr = _currentType->GetGUID(MF_MT_MAJOR_TYPE, outGuidMajorType);
        }
    }
    return hr;
}

HRESULT MFTStreamSink::onDispatchWorkItem(IMFAsyncResult* asyncResult)
{
    if (!asyncResult) {
        return E_INVALIDARG;
    }
    HRESULT hr = S_OK;
    CComPtr<IMFSample> sample;
    WrlComPtr<IUnknown> state;
    hr = asyncResult->GetState(&state);
    if (SUCCEEDED(hr)) {
        //const webrtc::MutexLock locker(&_lock);
        WrlComPtr<IAsyncStreamSinkOperation> sinkOp;
        hr = state.As(&sinkOp);
        if (SUCCEEDED(hr)) {
            Operation op;
            hr = sinkOp->GetOp(&op);
            if (SUCCEEDED(hr)) {
                switch (op) {
                    case Operation::Start:
                        hr = QueueEvent(MEStreamSinkStarted, GUID_NULL, S_OK, NULL);
                        if (SUCCEEDED(hr)) {
                            {
                                const webrtc::MutexLock locker(&_lock);
                                sample = processSamplesFromQueue();
                            }
                            if (!sample) {
                                hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, S_OK, NULL);
                            }
                        }
                        break;
                    case Operation::Stop:
                        {
                            const webrtc::MutexLock locker(&_lock);
                            dropSamplesFromQueue();
                        }
                        hr = QueueEvent(MEStreamSinkStopped, GUID_NULL, S_OK, NULL);
                        break;
                    case Operation::ProcessSample:
                        hr = QueueEvent(MEStreamSinkRequestSample, GUID_NULL, S_OK, NULL);
                        if (SUCCEEDED(hr)) {
                            const webrtc::MutexLock locker(&_lock);
                            sample = processSamplesFromQueue();
                        }
                        break;
                    case Operation::PlaceMarker:
                        {
                            PROPVARIANT propVariant;
                            PropVariantInit(&propVariant);
                            if (SUCCEEDED(sinkOp->GetPropVariant(&propVariant))) {
                                hr = QueueEvent(MEStreamSinkMarker, GUID_NULL, S_OK, &propVariant);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    if (SUCCEEDED(hr) && sample) {
        _callback.invoke(&MFTEncodingCallback::onEncoded, std::move(sample));
    }
    return hr;
}

HRESULT MFTStreamSink::validateOperation(Operation op) const
{
    if (_validStateMatrix[static_cast<uint16_t>(_state)][static_cast<uint16_t>(op)]) {
        return S_OK;
    }
    if (State::TypeNotSet == _state) {
        return MF_E_NOT_INITIALIZED;
    }
    return MF_E_INVALIDREQUEST;
}

HRESULT MFTStreamSink::queueAsyncOperation(Operation op, const PROPVARIANT* propVariant)
{
    WrlComPtr<IUnknown> sinkOp;
    HRESULT hr = Microsoft::WRL::MakeAndInitialize<AsyncStreamSinkOperation>(&sinkOp, op, propVariant);
    if (SUCCEEDED(hr)) {
        hr = MFPutWorkItem2(_workQueueId, 0, &_workQueueCb, sinkOp.Get());
    }
    return hr;
}

HRESULT MFTStreamSink::mediaTypeSupported(IMFMediaType* mediaType) const
{
    if (!mediaType) {
        return E_INVALIDARG;
    }
    GUID majorType = GUID_NULL;
    HRESULT hr = mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
    if (SUCCEEDED(hr) && majorType != MFMediaType_Video && majorType != MFMediaType_Audio) {
        hr = MF_E_INVALIDTYPE;
    }
    if (SUCCEEDED(hr) && _currentType) {
        GUID minorType = GUID_NULL;
        if (FAILED(mediaType->GetGUID(MF_MT_SUBTYPE, &minorType)) || minorType != _currentSubtype) {
            hr = MF_E_INVALIDTYPE;
        }
    }
    return hr;
}

HRESULT MFTStreamSink::processFormatChange()
{
    return queueAsyncOperation(Operation::SetMediaType);
}

CComPtr<IMFSample> MFTStreamSink::processSamplesFromQueue()
{
    if (!_sampleQueue.empty()) {
        auto sample = std::move(_sampleQueue.front());
        _sampleQueue.pop_front();
        return sample;
    }
    return {};
}

void MFTStreamSink::dropSamplesFromQueue()
{
    _sampleQueue.clear();
}

HRESULT MFTStreamSink::AsyncStreamSinkOperation::
    RuntimeClassInitialize(Operation op, const PROPVARIANT* propVariant) 
{
    _op = op;
    if (propVariant) {
         return PropVariantCopy(&_propVariant, propVariant);
    }
    PropVariantInit(&_propVariant);
    return S_OK;
}

HRESULT MFTStreamSink::AsyncStreamSinkOperation::GetOp(Operation* op) 
{
    if (!op) {
        return E_INVALIDARG;
    }
    *op = _op;
    return S_OK;
}

HRESULT MFTStreamSink::AsyncStreamSinkOperation::GetPropVariant(PROPVARIANT* propVariant) 
{
    if (!propVariant) {
        return E_INVALIDARG;
    }
    return PropVariantCopy(propVariant, &_propVariant);
}

} // namespace LiveKitCpp