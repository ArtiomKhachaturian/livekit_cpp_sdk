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
#pragma once // LocalParticipantImpl.h
#ifdef WEBRTC_AVAILABLE
#include "ParticipantImpl.h"
#include "CameraTrackImpl.h"
#include "LocalAudioTrackImpl.h"
#include "LocalParticipant.h"
#include "ParticipantListener.h"
#include "SafeObj.h"
#include <atomic>
#include <vector>

namespace webrtc {
class RtpSenderInterface;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class TrackManager;
class PeerConnectionFactory;
struct TrackPublishedResponse;
struct TrackUnpublishedResponse;

class LocalParticipantImpl : public ParticipantImpl<ParticipantListener, LocalParticipant>
{
public:
    LocalParticipantImpl(TrackManager* manager,
                         PeerConnectionFactory* pcf,
                         const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipantImpl() final;
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> tracks() const;
    LocalTrack* track(const std::string& id, bool cid);
    LocalTrack* track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    // impl of ParticipantImpl<>
    void setInfo(const ParticipantInfo& info) final;
    // impl. of Participant
    std::string sid() const final { return _sid(); }
    std::string identity() const final { return _identity(); }
    std::string name() const final { return _name(); }
    std::string metadata() const final { return _metadata(); }
    ParticipantKind kind() const final { return _kind; }
    // impl. of LocalParticipant
    CameraTrackImpl& camera() final { return _camera; }
    const CameraTrackImpl& camera() const final { return _camera; }
    LocalAudioTrackImpl& microphone() final { return _microphone; }
    const LocalAudioTrackImpl& microphone() const final { return _microphone; }
private:
    LocalAudioTrackImpl _microphone;
    CameraTrackImpl _camera;
    Bricks::SafeObj<std::string> _sid;
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _name;
    Bricks::SafeObj<std::string> _metadata;
    std::atomic<ParticipantKind> _kind = ParticipantKind::Standard;
};

} // namespace LiveKitCpp
#endif
