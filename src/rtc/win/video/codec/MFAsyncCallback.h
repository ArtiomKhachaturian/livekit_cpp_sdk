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
#pragma once // MFAsyncCallback.h
#include <mfidl.h>

namespace LiveKitCpp 
{

template <class TParent>
class MFAsyncCallback : public IMFAsyncCallback 
{
public:
    using InvokeFn = HRESULT(TParent::*)(IMFAsyncResult*);
public:
    MFAsyncCallback(TParent* parent, InvokeFn fn);
    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() final;
    ULONG STDMETHODCALLTYPE Release() final;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppv) final;
    // IMFAsyncCallback methods
    HRESULT STDMETHODCALLTYPE GetParameters(DWORD*, DWORD*) final;
    HRESULT STDMETHODCALLTYPE Invoke(IMFAsyncResult* asyncResult) final;
private:
    TParent* const _parent;
    const InvokeFn _invokeFn;
};

template <class TParent>
inline MFAsyncCallback<TParent>::MFAsyncCallback(TParent* parent, InvokeFn invokeFn)
    : _parent(parent)
    , _invokeFn(invokeFn) 
{
}

template <class TParent>
inline ULONG MFAsyncCallback<TParent>::AddRef()
{
    // Delegate to parent class.
    return _parent ? _parent->AddRef() : 0L;
}

template <class TParent>
inline ULONG MFAsyncCallback<TParent>::Release() 
{
    // Delegate to parent class.
    return _parent ? _parent->Release() : 0L;
}

template <class TParent>
inline HRESULT MFAsyncCallback<TParent>::QueryInterface(REFIID iid, void** ppv)
{
    if (!ppv) {
        return E_POINTER;
    }
    if (iid == __uuidof(IUnknown)) {
        *ppv = static_cast<IUnknown*>(static_cast<IMFAsyncCallback*>(this));
    }
    else if (iid == __uuidof(IMFAsyncCallback)) {
        *ppv = static_cast<IMFAsyncCallback*>(this);
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

template <class TParent>
inline HRESULT MFAsyncCallback<TParent>::GetParameters(DWORD*, DWORD*)
{
    // Implementation of this method is optional.
    return E_NOTIMPL;
}

template <class TParent>
inline HRESULT MFAsyncCallback<TParent>::Invoke(IMFAsyncResult* asyncResult) 
{
    if (_parent && _invokeFn) {
        return (_parent->*_invokeFn)(asyncResult);
    }
    return E_POINTER;
}

} // namespace LiveKitCpp 