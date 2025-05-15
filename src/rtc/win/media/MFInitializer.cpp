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
#include "MFInitializer.h"
#include "Utils.h"
#include <mfapi.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

namespace LiveKitCpp 
{

class MFInitializer::Impl : public ComStatus
{
public:
    Impl();
    ~Impl();
};

ComStatus MFInitializer::initializeForThisThread()
{
    static thread_local const Impl impl;
    return impl;
}

MFInitializer::Impl::Impl()
    : ComStatus(::MFStartup(MF_VERSION, MFSTARTUP_LITE))
{
}

MFInitializer::Impl::~Impl()
{
    if (ok()) {
        ::MFShutdown();
    }
}

} // namespace LiveKitCpp