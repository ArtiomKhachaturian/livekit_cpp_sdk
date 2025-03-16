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
#pragma once // RemoteParticipantImpl.h
#include "ParticipantImpl.h"
#include "RemoteParticipant.h"

namespace LiveKitCpp
{

class RemoteParticipantImpl : public ParticipantImpl<RemoteParticipant>
{
public:
    RemoteParticipantImpl() = default;
    size_t audioTracksCount() const final;
    size_t videoTracksCount() const final;
    std::shared_ptr<AudioTrack> audioTrack(size_t index) const final;
    std::shared_ptr<VideoTrack> videoTrack(size_t index) const final;
};

} // namespace LiveKitCpp
