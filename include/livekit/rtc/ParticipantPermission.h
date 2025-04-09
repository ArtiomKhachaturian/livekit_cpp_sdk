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
#pragma once // ParticipantPermission.h
#include "livekit/rtc/TrackSource.h"
#include <vector>

namespace LiveKitCpp
{

struct ParticipantPermission
{
    // allow participant to subscribe to other tracks in the room
    bool _canSubscribe = {};
    // allow participant to publish new tracks to room
    bool _canPublish = {};
    // allow participant to publish data
    bool _canPublish_data = {};
    // sources that are allowed to be published
    std::vector<TrackSource> _canPublishSources;
    // indicates that it's hidden to others
    bool _hidden = {};
    // indicates it's a recorder instance
    [[deprecated("Use ParticipantInfo._kind instead.")]] bool _recorder = {};
    // indicates that participant can update own metadata and attributes
    bool _canUpdateMetadata = {};
    // indicates that participant is an agent
    [[deprecated("Use ParticipantInfo._kind instead.")]] bool _agent = {};
    // if a participant can subscribe to metrics
    bool _canSubscribeMetrics = {};
};

} // namespace LiveKitCpp
