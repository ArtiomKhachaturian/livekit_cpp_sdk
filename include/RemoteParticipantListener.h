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
#pragma once // RemoteParticipantListener.h
#include "ParticipantListener.h"
#include <string>

namespace LiveKitCpp
{

class RemoteParticipant;

class RemoteParticipantListener : public ParticipantListener
{
public:
    virtual void onAudioTrackAdded(const RemoteParticipant* /*participant*/,
                                   const std::string& /*sid*/) {}
    virtual void onAudioTrackRemoved(const RemoteParticipant* /*participant*/,
                                     const std::string& /*sid*/) {}
    virtual void onVideoTrackAdded(const RemoteParticipant* /*participant*/,
                                   const std::string& /*sid*/) {}
    virtual void onVideoTrackRemoved(const RemoteParticipant* /*participant*/,
                                     const std::string& /*sid*/) {}
protected:
    virtual ~RemoteParticipantListener() = default;
};

} // namespace LiveKitCpp
