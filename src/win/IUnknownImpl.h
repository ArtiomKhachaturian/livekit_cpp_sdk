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
#include <mutex>
#include <type_traits>
#include <unknwn.h>

namespace LiveKitCpp 
{

template <class... TBaseComInterfaces>
class IUnknownImpl : public TBaseComInterfaces...
{
    using Mutex = std::recursive_mutex;
    using MutexLock = std::lock_guard<Mutex>;
    static_assert((std::is_base_of_v<IUnknown, TBaseComInterfaces> && ...), 
                  "TBaseComInterfaces must be derived from IUnknown");
public:
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) override;
protected:
    IUnknownImpl() = default;
    // IUnknown has no virtual destructor
    virtual void releaseThis() = 0;
private:
    ULONG decrementRef();
    template <class TComInterface>
    static bool isUuidOf(REFIID riid, TComInterface*);
    template <class TComInterface, class... TRestComInterfaces>
    static bool isUuidOf(REFIID riid, TComInterface* comInterface,
                         TRestComInterfaces... restComInterfaces);
private:
    Mutex _refMutex;
    LONG _ref = 0LL;
};

template <class... TBaseComInterfaces>
inline ULONG IUnknownImpl<TBaseComInterfaces...>::AddRef()
{
    const MutexLock lock(_refMutex);
    return ++_ref;
}

template <class... TBaseComInterfaces>
inline ULONG IUnknownImpl<TBaseComInterfaces...>::Release()
{
    const ULONG refCount = decrementRef();
    if (0UL == refCount) {
        releaseThis();
    }
    return refCount;
}

template <class... TBaseComInterfaces>
inline HRESULT IUnknownImpl<TBaseComInterfaces...>::QueryInterface(REFIID riid, VOID** ppvInterface)
{
    HRESULT hr = E_INVALIDARG;
    if (ppvInterface) {
        *ppvInterface = nullptr;
        if (IID_IUnknown == riid || isUuidOf(riid, static_cast<TBaseComInterfaces*>(this)...)) {
            *ppvInterface = (void*)this;
            AddRef();
            hr = NOERROR;
        }
        else {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}

template <class... TBaseComInterfaces>
inline ULONG IUnknownImpl<TBaseComInterfaces...>::decrementRef()
{
    const MutexLock lock(_refMutex);
    if (_ref > 0UL) {
        --_ref;
    }
    return _ref;
}

template <class... TBaseComInterfaces>
template <class TComInterface>
inline bool IUnknownImpl<TBaseComInterfaces...>::isUuidOf(REFIID riid, TComInterface*)
{
    return riid == __uuidof(TComInterface);
}

template <class... TBaseComInterfaces>
template <class TComInterface, class... TRestComInterfaces>
inline bool IUnknownImpl<TBaseComInterfaces...>::isUuidOf(REFIID riid, 
                                                          TComInterface* comInterface, 
                                                          TRestComInterfaces... restComInterfaces)
{
    return isUuidOf(riid, comInterface) || isUuidOf(riid, restComInterfaces...);
}

} // namespace LiveKitCpp