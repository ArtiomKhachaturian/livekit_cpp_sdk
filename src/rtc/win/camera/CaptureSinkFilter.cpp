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
#ifdef WEBRTC_WIN
#include "CaptureSinkFilter.h"
#include "CaptureInputPin.h"

DEFINE_GUID(CLSID_SINKFILTER, 0x88cdbbdc, 0xa73b, 0x4afa, 0xac, 0xbf, 0x15, 0xd5, 0xe2, 0xce, 0x12, 0xc3);

namespace LiveKitCpp 
{

class CaptureSinkFilter::PinsEnumerator : public IUnknownImpl<IEnumPins>
{
public:
    PinsEnumerator(IPin* pin);
    // impl. of IEnumPins
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Clone(IEnumPins**) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Next(ULONG count, IPin** pins, ULONG* fetched) final;
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Skip(ULONG) final { return E_NOTIMPL; }
    __declspec(nothrow) HRESULT STDMETHODCALLTYPE Reset() final;
protected:
    void releaseThis() final;
private:
    const CComPtr<IPin> _pin;
    int _pos = 0;
};

CaptureSinkFilter::CaptureSinkFilter(CapturedFrameReceiver* receiver,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : _receiver(receiver)
    , _inputPin(new CaptureInputPin(this, logger))
{
}

CaptureSinkFilter::~CaptureSinkFilter()
{
}

HRESULT CaptureSinkFilter::setRequestedCapability(const webrtc::VideoCaptureCapability& capability)
{
    return _inputPin->SetRequestedCapability(capability);
}

void CaptureSinkFilter::notifyEvent(long code, LONG_PTR param1, LONG_PTR param2)
{
    LOCK_READ_SAFE_OBJ(_sink);
    if (const auto& sink = _sink.constRef()) {
        if (EC_COMPLETE == code) {
            param2 = reinterpret_cast<LONG_PTR>(static_cast<IBaseFilter*>(this));
        }
        sink->Notify(code, param1, param2);
    }
}

bool CaptureSinkFilter::isStopped() const
{
    return FILTER_STATE::State_Stopped == _state.load(std::memory_order_relaxed);
}

void CaptureSinkFilter::deliverFrame(BYTE* buffer, DWORD actualBufferLen,
                                     DWORD totalBufferLen, const CComPtr<IMediaSample>& sample,
                                     const webrtc::VideoCaptureCapability& frameInfo)
{
    if (_receiver) {
        _receiver->deliverFrame(buffer, actualBufferLen, totalBufferLen, sample, frameInfo);
    }
}

HRESULT CaptureSinkFilter::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = IUnknownImpl<IBaseFilter>::QueryInterface(riid, ppv);
    if (E_NOINTERFACE == hr) {
        if (ppv) {
            if (IID_IPersist == riid || IID_IMediaFilter == riid) {
                *ppv = (void*)this;
                AddRef();
                hr = S_OK;
            }
        } else {
            hr = E_POINTER;
        }
    }
    return hr;
}

HRESULT CaptureSinkFilter::GetClassID(CLSID* clsid)
{
    if (clsid) {
        *clsid = CLSID_SINKFILTER;
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CaptureSinkFilter::GetState(DWORD, FILTER_STATE* state)
{
    if (state) {
        *state = _state.load(std::memory_order_relaxed);
    }
    return E_POINTER;
}

HRESULT CaptureSinkFilter::Pause()
{
    _state = FILTER_STATE::State_Paused;
    return S_OK;
}

HRESULT CaptureSinkFilter::Run(REFERENCE_TIME)
{
    if (FILTER_STATE::State_Stopped == _state.load(std::memory_order_relaxed)) {
        Pause();
    }
    if (FILTER_STATE::State_Running != _state.exchange(FILTER_STATE::State_Running)) {
        _inputPin->notifyThatFilterActivated(true);
    }
    return S_OK;
}

HRESULT CaptureSinkFilter::Stop()
{
    if (FILTER_STATE::State_Stopped != _state.exchange(FILTER_STATE::State_Stopped)) {
        _inputPin->notifyThatFilterActivated(false);
    }
    return S_OK;
}

HRESULT CaptureSinkFilter::EnumPins(IEnumPins** pins)
{
    if (pins) {
        *pins = new PinsEnumerator(_inputPin);
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CaptureSinkFilter::FindPin(LPCWSTR, IPin**)
{
    // there's no ID assigned to our input pin, so looking it up based on one
    // is pointless (and in practice, this method isn't being used)
    return VFW_E_NOT_FOUND;
}

HRESULT CaptureSinkFilter::QueryFilterInfo(FILTER_INFO* info)
{
    if (info) {
        *info = _info();
        if (info->pGraph) {
            info->pGraph->AddRef();
        }
        return S_OK;
    }
    return E_POINTER;
}

HRESULT CaptureSinkFilter::JoinFilterGraph(IFilterGraph* graph, LPCWSTR name)
{
    HRESULT hr = VFW_E_NOT_STOPPED;
    if (isStopped()) {
        hr = S_OK;
        LOCK_WRITE_SAFE_OBJ(_info);
        _info.ref().pGraph = graph;
        {
            LOCK_WRITE_SAFE_OBJ(_sink);
            _sink = nullptr;
            if (graph) {
                graph->QueryInterface(&_sink.ref());
            }
        }
        _info.ref().achName[0] = L'\0';
        if (name) {
            hr = ::StringCchCopyW(_info.ref().achName,
                                  sizeof(_info.ref().achName) / sizeof(_info.ref().achName[0]),
                                  name);
        }
    }
    return hr;
}

CaptureSinkFilter::PinsEnumerator::PinsEnumerator(IPin* pin)
    : _pin(pin)
{
}

HRESULT CaptureSinkFilter::PinsEnumerator::Next(ULONG count, IPin** pins, ULONG* fetched)
{
    if (pins) {
        if (count > 0UL) {
            if (_pos > 0) {
                if (fetched) {
                    *fetched = 0UL;
                }
                return S_FALSE;
            }
            ++_pos;
            pins[0] = _pin;
            pins[0]->AddRef();
            if (fetched) {
                *fetched = 1UL;
            }
            return count == 1UL ? S_OK : S_FALSE;
        }
        return E_INVALIDARG;
    }
    return E_POINTER;
}

HRESULT CaptureSinkFilter::PinsEnumerator::Reset()
{
    _pos = 0;
    return S_OK;
}

void CaptureSinkFilter::PinsEnumerator::releaseThis()
{
    delete this;
}

} // namespace LiveKitCpp
#endif