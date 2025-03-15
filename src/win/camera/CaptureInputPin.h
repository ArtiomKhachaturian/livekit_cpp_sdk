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
#ifdef WEBRTC_WIN
#include "Loggable.h"
#include "IUnknownImpl.h"
#include "SafeComPtr.h"
#include <atomic>
#include <dshow.h>
#include <modules/video_capture/video_capture_defines.h>

namespace LiveKitCpp 
{

class CaptureSinkFilter;

class CaptureInputPin : public Bricks::LoggableS<IUnknownImpl<IMemInputPin, IPin>>
{
    class MediaTypesEnum;
    using Base = Bricks::LoggableS<IUnknownImpl<IMemInputPin, IPin>>;
public:
    CaptureInputPin(CaptureSinkFilter* filter, const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CaptureInputPin();
    HRESULT SetRequestedCapability(const webrtc::VideoCaptureCapability& capability);
    void notifyThatFilterActivated(bool activated);
    // IPin
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Connect(IPin* receivePin, const AM_MEDIA_TYPE* mediaType) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin* connector, const AM_MEDIA_TYPE* mediaType) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Disconnect() final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE ConnectedTo(IPin** pin) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE* mediaType) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO* info) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION* pinDir) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryId(LPWSTR* id) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE* mediaType) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes** outputEnumerator) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin**, ULONG*) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE EndOfStream() final { return S_OK; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE BeginFlush() final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE EndFlush() final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME, REFERENCE_TIME, double) final { return S_OK; }
    // IMemInputPin
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE GetAllocator(IMemAllocator** allocator) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE NotifyAllocator(IMemAllocator* allocator, BOOL readOnly) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(ALLOCATOR_PROPERTIES*) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Receive(IMediaSample* sample) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE ReceiveMultiple(IMediaSample** samples, long count, long* processed) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE ReceiveCanBlock() final { return S_FALSE; }
protected:
    // impl. of IUnknownImpl
    void releaseThis() final { delete this; }
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final;
private:
    CaptureSinkFilter* filter() const;
    void resetMediaType(const AM_MEDIA_TYPE* newMediaType = nullptr);
    HRESULT checkDirection(IPin* pin) const;
    void clearAllocator(bool decommit);
    HRESULT attemptConnection(IPin* receivePin, const AM_MEDIA_TYPE* mediaType);
    IEnumMediaTypes* createMediaTypesEnumerator() const;
    bool hasFlushing() const { return _flushing.load(std::memory_order_relaxed); }
    bool hasRuntimeError() const { return _runtimeError.load(std::memory_order_relaxed); }
    std::vector<AM_MEDIA_TYPE*> determineCandidateFormats(IPin* receivePin, const AM_MEDIA_TYPE* mediaType) const;
private:
    const PIN_INFO _info;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _requestedCapability;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _resultingCapability;
    SafeComPtr<IMemAllocator> _allocator;
    SafeComPtr<IPin> _receivePin;
    Bricks::SafeObj<AM_MEDIA_TYPE> _mediaType;
    std::atomic<DWORD> _captureThreadId = 0LL;
    std::atomic_bool _flushing = false;
    std::atomic_bool _runtimeError = false;
};

} // namespace LiveKitCpp
#endif