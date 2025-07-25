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
#pragma once // UserPacket.h
#include <string>
#include <optional>
#include <vector>

namespace LiveKitCpp
{

struct UserPacket
{
    // participant ID of user that sent the message
    [[deprecated]] std::string _participantSid;
    [[deprecated]] std::string _participantIdentity;
    // user defined payload
    std::string _payload;
    // the ID of the participants who will receive the message (sent to all by default)
    [[deprecated]] std::vector<std::string> _destinationSids;
    // identities of participants who will receive the message (sent to all by default)
    [[deprecated]] std::vector<std::string> _destinationIdentities;
    // topic under which the message was published
    std::optional<std::string> _topic;
    // Unique ID to indentify the message
    std::optional<std::string> _id;
    // start and end time allow relating the message to specific media time
    std::optional<uint64_t> _startTime;
    std::optional<uint64_t> _endTime;
    // added by SDK to enable de-duping of messages, for INTERNAL USE ONLY
    std::string _nonce;
};

}
