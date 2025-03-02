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
#pragma once // ParticipantKind.h

namespace LiveKitCpp
{

enum class ParticipantKind
{
    // Standard Participants, E.G. Web Clients
    Standard = 0,
    // Only Ingests Streams
    Ingress = 1,
    // Only Consumes Streams
    Egress = 2,
    // Sip Participants
    Sip = 3,
    // Livekit Agents
    Agent = 4,
};

} // namespace LiveKitCpp
