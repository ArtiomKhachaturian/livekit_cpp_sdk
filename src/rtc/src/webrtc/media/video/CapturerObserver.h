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
#pragma once // CapturerObserver.h
#include "CapturerState.h"
#include <string>

namespace LiveKitCpp
{

class CapturerObserver
{
public:
    virtual void onStateChanged(CapturerState state) = 0;
    virtual void onCapturingFatalError(const std::string& /*details*/ = {}) {} // during the streaming
protected:
    ~CapturerObserver() = default;
};

} // namespace LiveKitCpp
