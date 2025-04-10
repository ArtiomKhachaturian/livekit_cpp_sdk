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
#pragma once // Track.h
#include "livekit/rtc/TrackType.h"
#include "livekit/rtc/TrackSource.h"
#include "livekit/rtc/EncryptionType.h"
#include "livekit/rtc/BackupCodecPolicy.h"
#include "livekit/stats/StatsSource.h"
#include <string>

namespace LiveKitCpp
{

class MediaEventsListener;

class Track : public StatsSource
{
public:
    ~Track() override = default;
    virtual EncryptionType encryption() const = 0;
    virtual BackupCodecPolicy backupCodecPolicy() const { return BackupCodecPolicy::Regression; }
    // server track ID if any
    virtual std::string sid() const = 0;
    // track ID
    virtual std::string id() const = 0;
    // track name
    virtual std::string name() const = 0;
    // notify that remote user was muted own track (for [remote] tracks)
    // or your track (for local tracks) on his side
    virtual bool remoteMuted() const = 0;
    // local or remote
    virtual bool remote() const noexcept = 0;
    // live or ended, a track will never be live again after becoming ended
    virtual bool live() const = 0;
    // type
    virtual TrackType type() const = 0;
    // source
    virtual TrackSource source() const = 0;
    // mute/unmute state
    virtual void mute(bool mute = true) = 0;
    virtual bool muted() const = 0;
    void unmute() { mute(false); }
    // track events
    virtual void addListener(MediaEventsListener* listener) = 0;
    virtual void removeListener(MediaEventsListener* listener) = 0;
};

} // namespace LiveKitCpp
