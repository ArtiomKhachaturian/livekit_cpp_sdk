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
#pragma once // IUnknownImpl.h
#include <type_traits>
#include <unknwn.h>

namespace LiveKitCpp 
{

template <class... TBaseComInterfaces>
class IUnknownImpl : public TBaseComInterfaces...
{
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
    template <class TComInterface>
    static bool isUuidOf(REFIID riid, TComInterface*);
    template <class TComInterface, class... TRestComInterfaces>
    static bool isUuidOf(REFIID riid, TComInterface* comInterface,
                         TRestComInterfaces... restComInterfaces);
private:
    volatile LONG _refCount = 0;
};

template <class... TBaseComInterfaces>
inline ULONG IUnknownImpl<TBaseComInterfaces...>::AddRef()
{
    return static_cast<ULONG>(::InterlockedIncrement(&_refCount));
}

template <class... TBaseComInterfaces>
inline ULONG IUnknownImpl<TBaseComInterfaces...>::Release()
{
    const auto refCount = ::InterlockedDecrement(&_refCount);
    if (refCount <= 0) {
        releaseThis();
        return 0U;
    }
    return static_cast<ULONG>(refCount);
}

template <class... TBaseComInterfaces>
inline HRESULT IUnknownImpl<TBaseComInterfaces...>::QueryInterface(REFIID riid, VOID** ppvInterface)
{
    if (ppvInterface) {
        *ppvInterface = nullptr;
        if (IID_IUnknown == riid || isUuidOf(riid, static_cast<TBaseComInterfaces*>(this)...)) {
            *ppvInterface = (void*)this;
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }
    return E_INVALIDARG;
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