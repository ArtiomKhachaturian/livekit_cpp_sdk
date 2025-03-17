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
#pragma once // RemoteParticipants.h
#include "RemoteParticipant.h"
#include <memory>
#include <string>

namespace LiveKitCpp
{

class RemoteParticipantsListener;

class RemoteParticipants
{
public:
    virtual ~RemoteParticipants() = default;
    virtual void addListener(RemoteParticipantsListener* listener) = 0;
    virtual void removeListener(RemoteParticipantsListener* listener) = 0;
    virtual size_t count() const = 0;
    // given participant by index or server ID
    virtual std::shared_ptr<RemoteParticipant> at(size_t index) const = 0;
    virtual std::shared_ptr<RemoteParticipant> at(const std::string& sid) const = 0;
};

} // namespace LiveKitCpp
