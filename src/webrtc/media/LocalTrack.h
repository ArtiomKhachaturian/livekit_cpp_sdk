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
#include <api/media_types.h>
#include <atomic>
#include <string>

namespace webrtc {
class MediaStreamTrackInterface;
class RtpSenderInterface;
}

namespace LiveKitCpp
{

class LocalTrackManager;
struct AddTrackRequest;

enum class SetSenderResult
{
    Accepted,
    Rejected,
    NotMatchedToRequest
};

class LocalTrack
{
public:
    virtual ~LocalTrack();
    // client track ID, equal to WebRTC track ID
    const std::string& cid() const noexcept { return _cid; }
    // track name
    const std::string& name() const noexcept { return _name; }
    // server track ID, received from TrackPublishedResponse
    std::string sid() const noexcept { return _sid; }
    void setSid(const std::string& sid) { _sid(sid); }
    // request media track creation if needed
    void mute(bool mute);
    bool muted() const noexcept { return _muted; }
    SetSenderResult setTrackSender(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    bool resetTrackSender(bool full = false);
    // type
    virtual cricket::MediaType mediaType() const noexcept = 0;
    // for publishing
    virtual bool fillRequest(AddTrackRequest& request) const;
protected:
    LocalTrack(std::string name, LocalTrackManager* manager);
    LocalTrackManager* manager() const noexcept { return _manager; }
    virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        createMediaTrack(const std::string& id) = 0;
private:
    void muteSender(bool mute, bool notify) const;
    bool accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const;
    bool accept(const std::string& id) const { return !id.empty() && id == _cid; }
private:
    const std::string _cid;
    const std::string _name;
    LocalTrackManager* const _manager;
    Bricks::SafeObj<std::string> _sid;
    std::atomic_bool _muted = true;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
    bool _requested = false; // united protection with [_sender]
};

} // namespace LiveKitCpp
