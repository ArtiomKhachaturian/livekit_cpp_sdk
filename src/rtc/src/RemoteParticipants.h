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
#pragma once // RemoteParticipants.h
#include "Loggable.h"
#include "NonBindedRtpReceivers.h"
#include "SafeObj.h"
#include "livekit/rtc/RemoteParticipantListener.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <vector>

namespace webrtc {
class MediaStreamTrackInterface;
class RtpTransceiverInterface;
class RtpReceiverInterface;
}

namespace LiveKitCpp
{

class E2ESecurityFactory;
class RemoteParticipantsListener;
class RemoteParticipantImpl;
class RemoteParticipant;
struct ParticipantInfo;
enum class TrackType;

class RemoteParticipants : private Bricks::LoggableS<RemoteParticipantListener>
{
    using Participants = std::vector<std::shared_ptr<RemoteParticipantImpl>>;
public:
    RemoteParticipants(bool autoSubscribe, E2ESecurityFactory* securityFactory,
                       RemoteParticipantsListener* listener,
                       const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RemoteParticipants();
    bool setRemoteSideTrackMute(const std::string& sid, bool mute);
    void setInfo(const std::vector<ParticipantInfo>& infos = {});
    void updateInfo(const std::vector<ParticipantInfo>& infos);
    bool addMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                  std::string trackSid, std::string participantSid = {});
    bool removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    void reset();
    size_t count() const;
    std::shared_ptr<RemoteParticipantImpl> at(size_t index) const;
    std::shared_ptr<RemoteParticipantImpl> at(const std::string& sid) const;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    void requestSubscriptionChanges(const RemoteParticipantImpl* participant, bool subscribe,
                                    std::vector<std::string> trackSids = {}) const;
    // non thread-safe to [_participants]
    std::vector<ParticipantInfo> infos() const;
    // service methods, non thread-safe to [_participants]
    void addParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant);
    void removeParticipant(const std::string& sid);
    void clearParticipants();
    void dispose(const std::shared_ptr<RemoteParticipantImpl>& participant);
    std::optional<size_t> participantIndexBySid(const std::string& sid) const;
    // impl. of RemoteParticipantListener
    void onRemoteTrackAdded(const RemoteParticipant* participant, TrackType,
                            EncryptionType, const std::string& sid) final;
private:
    const bool _autoSubscribe;
    E2ESecurityFactory* const _securityFactory;
    RemoteParticipantsListener* const _listener;
    const std::shared_ptr<NonBindedRtpReceivers> _nonBindedReceivers;
    Bricks::SafeObj<Participants> _participants;
};

} // namespace LiveKitCpp
