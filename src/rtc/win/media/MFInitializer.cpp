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

MFInitializer::MFInitializer(bool multiThreadedModel, bool liteMode)
    : ComStatus(initializeComForThisThread(multiThreadedModel))
{
    if (ok()) {
        setStatus(::MFStartup(MF_VERSION, liteMode ? MFSTARTUP_LITE : MFSTARTUP_FULL));
    }
}

MFInitializer::MFInitializer(MFInitializer&& tmp) noexcept
    : ComStatus(std::move(tmp))
{
}

MFInitializer::~MFInitializer()
{
    if (ok()) {
        ::MFShutdown();
    }
}

MFInitializer& MFInitializer::operator=(MFInitializer&& tmp) noexcept
{
    assign(std::move(tmp));
    return *this;
}

} // namespace LiveKitCpp