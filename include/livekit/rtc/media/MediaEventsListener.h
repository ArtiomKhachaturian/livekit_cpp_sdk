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
#pragma once // MediaEventsListener.h
#include <string>

namespace LiveKitCpp
{

class MediaEventsListener
{
public:
    virtual void onMuteChanged(const std::string& /*id*/, bool /*mute*/) {}
    // below events related only to Track interface
    // notify that remote user was muted own track (for remote tracks)
    // or your track (for local tracks)
    virtual void onRemoteSideMuteChanged(const std::string& /*id*/, bool /*mute*/) {}
    // SID maybe changed only for local tracks, after joining
    virtual void onSidChanged(const std::string& /*id*/, const std::string& /*sid*/) {}
    // only for remote tracks & devices
    virtual void onNameChanged(const std::string& /*id*/, const std::string& /*name*/) {}
    // device events, only for local tracks & devices
    virtual void onMediaChanged(const std::string& /*id*/) {}
    virtual void onMediaCreationFailed(const std::string& /*id*/, const std::string& /*details*/ = {}) {}
    virtual void onMediaOptionsChanged(const std::string& /*id*/) {}
    virtual void onMediaStarted(const std::string& /*id*/) {}
    virtual void onMediaStartFailed(const std::string& /*id*/, const std::string& /*details*/ = {}) {}
    virtual void onMediaStopped(const std::string& /*id*/) {}
    virtual void onMediaStopFailed(const std::string& /*id*/, const std::string& /*details*/ = {}) {}
    // during the streaming
    virtual void onMediaFatalError(const std::string& /*id*/, const std::string& /*details*/ = {}) {}
protected:
    virtual ~MediaEventsListener() = default;
};

} // namespace LiveKitCpp
