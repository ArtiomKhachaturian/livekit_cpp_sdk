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
#pragma once // Participant.h
#include "rtc/ParticipantPermission.h"
#include "rtc/ParticipantKind.h"
#include "rtc/ParticipantState.h"
#include <string>

namespace LiveKitCpp
{

class Participant
{
public:
    virtual ~Participant() = default;
    virtual std::string sid() const = 0;
    virtual std::string identity() const = 0;
    virtual std::string name() const = 0;
    virtual std::string metadata() const = 0;
    virtual ParticipantState state() const = 0;
    virtual bool hasActivePublisher() const = 0;
    virtual ParticipantKind kind() const = 0;
    /* int64_t _joinedAt = {};
    uint32_t _version = {};
    std::optional<ParticipantPermission> _permission;
    std::string _region;
    std::unordered_map<std::string, std::string> _attributes;*/
};

} // namespace LiveKitCpp
