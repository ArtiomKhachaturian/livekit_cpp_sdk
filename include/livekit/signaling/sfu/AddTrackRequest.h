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
#pragma once // AddTrackRequest.h
#include "livekit/signaling/sfu/TrackType.h"
#include "livekit/signaling/sfu/TrackSource.h"
#include "livekit/signaling/sfu/VideoLayer.h"
#include "livekit/signaling/sfu/SimulcastCodec.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include "livekit/signaling/sfu/BackupCodecPolicy.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

struct AddTrackRequest
{
    // client ID of track, to match it when RTC track is received
    std::string _cid;
    std::string _name;
    TrackType _type = {};
    // to be deprecated in favor of layers
    uint32_t _width = {};
    uint32_t _height = {};
    // true to add track and initialize to muted
    bool _muted = {};
    // true if DTX (Discontinuous Transmission) is disabled for audio
    [[deprecated("deprecated in favor of AudioTrackFeature::NoDtx")]]
    bool _disableDtx = {};
    TrackSource _source = {};
    std::vector<VideoLayer> _layers;
    std::vector<SimulcastCodec> _simulcastCodecs;
    // server ID of track, publish new codec to exist track
    std::string _sid;
    [[deprecated("deprecated in favor of AudioTrackFeature::Stereo")]]
    bool _stereo = {};
    // true if RED (Redundant Encoding) is disabled for audio
    bool _disableRed = {};
    EncryptionType _encryption = {};
    // which stream the track belongs to, used to group tracks together.
    // if not specified, server will infer it from track source to bundle camera/microphone, screenshare/audio together
    std::string _stream;
    BackupCodecPolicy _backupCodecPolicy = {};
};

} // namespace LiveKitCpp
