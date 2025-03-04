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
#pragma once // UpdateParticipantMetadata.h
#include <string>
#include <unordered_map>

namespace LiveKitCpp
{

// update a participant's own metadata, name, or attributes
// requires canUpdateOwnParticipantMetadata permission
struct UpdateParticipantMetadata
{
    std::string _metadata;
    std::string _name;
    // attributes to update. it only updates attributes that have been set
    // to delete attributes, set the value to an empty string
    std::unordered_map<std::string, std::string> _attributes;
    uint32_t _requestId = {};
};

} // namespace LiveKitCpp
