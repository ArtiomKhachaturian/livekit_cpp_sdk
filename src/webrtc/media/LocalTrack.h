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
#pragma once // LocalTrack.h
#include "SafeScopedRefPtr.h"
#include "rtc/AddTrackRequest.h"
#include "Track.h"
#include <atomic>
#include <string>

namespace LiveKitCpp
{

class LocalTrackManager;

class LocalTrack : public Track
{
public:
    ~LocalTrack() override;
    void reset();
    // client track ID, equal to WebRTC track ID
    const std::string& cid() const noexcept { return _cid; }
    // track name
    const std::string& name() const noexcept { return _name; }
    void setSid(const std::string& sid) { _sid(sid); }
    void setPublished(bool error = false);
    // for publishing
    bool live() const noexcept;
    virtual void fillRequest(AddTrackRequest& request) const;
    // impl. of Track
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> raw() const final;
    std::string sid() const final { return _sid(); }
    // request media track creation if needed
    void mute(bool mute) final;
    bool muted() const final { return _muted; }
protected:
    LocalTrack(std::string name, LocalTrackManager* manager);
    LocalTrackManager* manager() const noexcept { return _manager; }
    virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        createMediaTrack(const std::string& id) = 0;
private:
    void notifyAboutMuted(bool mute) const;
private:
    const std::string _cid;
    const std::string _name;
    LocalTrackManager* const _manager;
    Bricks::SafeObj<std::string> _sid;
    std::atomic_bool _muted = true;
    SafeScopedRefPtr<webrtc::MediaStreamTrackInterface> _track;
    // under lock together with [_track]
    bool _pendingPublish = false;
};

} // namespace LiveKitCpp
