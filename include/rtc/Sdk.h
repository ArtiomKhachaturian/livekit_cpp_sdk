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
#pragma once // Sdk.h
#include "LiveKitClientExport.h"
#include <string>

namespace LiveKitCpp
{

enum class SDK
{
    Unknown = 0,
    JS = 1,
    Swift = 2,
    Android = 3,
    Flutter = 4,
    GO = 5,
    Unity = 6,
    ReactNative = 7,
    Rust = 8,
    Python = 9,
    CPP = 10,
    UnityWeb = 11,
    Node = 12,
};

LIVEKIT_CLIENT_API std::string toString(SDK sdk);

} // namespace LiveKitCpp
