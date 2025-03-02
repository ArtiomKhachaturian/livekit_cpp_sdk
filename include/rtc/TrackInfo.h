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
#pragma once // TrackInfo.h
#include "rtc/TrackSource.h"
#include "rtc/TimedVersion.h"
#include <optional>
#include <string>
#include <vector>

namespace LiveKitCpp
{

struct TrackInfo
{
    std::string _sid = {};
    //TrackType _type = {};
    std::string _name = {};
    bool _muted = {};
    // original width of video (unset for audio)
    // clients may receive a lower resolution version with simulcast
    uint32_t _width = {};
    // original height of video (unset for audio)
    uint32_t _height = {};
    // true if track is simulcasted
    bool _simulcast = {};
    // true if DTX (Discontinuous Transmission) is disabled for audio
    bool _disableDtx = {};
    // source of media
    TrackSource _source = {};
    //repeated VideoLayer _layers = 10;
    // mime type of codec
    std::string _mimeType;
    std::string _mid;
    //repeated SimulcastCodecInfo codecs = 13;
    bool _stereo = {};
    // true if RED (Redundant Encoding) is disabled for audio
    bool _disableRed = {};
    //Encryption.Type _encryption = {};
    std::string _stream;
    std::optional<TimedVersion> _version;
    //repeated AudioTrackFeature _audioFeatures;
    //BackupCodecPolicy _backupCodecPolicy = {};
};

} // namespace LiveKitCpp
