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
#ifdef _WIN32
#include "ScopedComInitializer.h"
#include <roapi.h>

#pragma comment(lib, "runtimeobject.lib")

namespace 
{

inline RO_INIT_TYPE initType(bool multiThreadedModel)
{
    return multiThreadedModel ? RO_INIT_MULTITHREADED : RO_INIT_SINGLETHREADED;
}

}

namespace LiveKitCpp
{

ScopedComInitializer::ScopedComInitializer(bool multiThreadedModel)
    : ComStatus(Windows::Foundation::Initialize(initType(multiThreadedModel)))
    , _differentApartment(RPC_E_CHANGED_MODE == status())
{
    // from https://learn.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-coinitializeex:
    // You should call Windows::Foundation::Initialize to initialize the thread instead of CoInitializeEx
    // if you want to use the Windows Runtime APIs or if you want to use both COMand Windows Runtime components.
    // Windows::Foundation::Initialize is sufficient to use for COM components
    if (_differentApartment) {
        setStatus(S_OK);
    }
}

ScopedComInitializer::~ScopedComInitializer()
{
    if (ok() && !_differentApartment) {
        Windows::Foundation::Uninitialize();
    }
}

} // namespace LiveKitCpp
#endif