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
#pragma once // DataStreamTextHeaderOperationType.h
#include "livekit/signaling/LiveKitSignalingExport.h"
#include <string>

namespace LiveKitCpp
{

// enum for operation types (specific to TextHeader)
enum class DataStreamTextHeaderOperationType
{
    Create,
    Update,
    Delete,
    Reaction
};

LIVEKIT_SIGNALING_API std::string toString(DataStreamTextHeaderOperationType type);
	
} // namespace LiveKitCpp
