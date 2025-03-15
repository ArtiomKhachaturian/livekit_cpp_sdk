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
#include "CapturedFrameReceiver.h"
#include "IUnknownImpl.h"
#include "SafeComPtr.h"
#include <atlbase.h> //CComPtr support
#include <strmif.h>
#include <atomic>
#include <memory>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp 
{

class CaptureInputPin;

// implement IBaseFilter (including IPersist and IMediaFilter).
class CaptureSinkFilter : public CapturedFrameReceiver, public IUnknownImpl<IBaseFilter>
{
    class PinsEnumerator;
public:
    CaptureSinkFilter(CapturedFrameReceiver* receiver, const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CaptureSinkFilter() override;
    HRESULT setRequestedCapability(const webrtc::VideoCaptureCapability& capability);
    void notifyEvent(long code, LONG_PTR param1, LONG_PTR param2);
    bool isStopped() const;
    // impl. of CapturedFrameReceiver
    void deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                      DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                      const webrtc::VideoCaptureCapability& frameInfo) final;
    // impl. of IUnknown
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) final;
    // impl. of IPersist
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE GetClassID(CLSID* clsid) final;
    // impl. of IMediaFilter
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE GetState(DWORD msecs, FILTER_STATE* state) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock*) final { return S_OK; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock**) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Pause() final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME start) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Stop() final;
    // impl. of IBaseFilter
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins** pins) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR id, IPin** pin) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO* info) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph* graph, LPCWSTR name) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR*) final { return E_NOTIMPL; }
protected:
    // impl. of IUnknownImpl
    void releaseThis() final { delete this; }
private:
    CapturedFrameReceiver* const _receiver;
    const CComPtr<CaptureInputPin> _inputPin;
    Bricks::SafeObj<FILTER_INFO> _info;
    SafeComPtr<IMediaEventSink> _sink;
    std::atomic<FILTER_STATE> _state = FILTER_STATE::State_Stopped;
};

} // namespace LiveKitCpp