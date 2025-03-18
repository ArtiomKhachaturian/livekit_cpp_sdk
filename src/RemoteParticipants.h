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
#ifdef WEBRTC_AVAILABLE
#include "DataChannelsStorage.h"
#include "SafeObj.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <vector>
#include <unordered_map>

namespace webrtc {
class MediaStreamTrackInterface;
class RtpTransceiverInterface;
class RtpReceiverInterface;
}

namespace LiveKitCpp
{

class TrackManager;
class RemoteParticipantsListener;
class RemoteParticipantImpl;
class RemoteParticipant;
struct ParticipantInfo;
enum class TrackType;

class RemoteParticipants : protected DataChannelsStorage<>
{
    using Participants = std::vector<std::shared_ptr<RemoteParticipantImpl>>;
    // key is sid
    using OrphanedTracks = std::unordered_map<std::string, rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>;
public:
    RemoteParticipants(TrackManager* trackManager,
                       RemoteParticipantsListener* listener,
                       const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RemoteParticipants() final { reset(); }
    void setInfo(const std::vector<ParticipantInfo>& infos = {});
    void updateInfo(const std::vector<ParticipantInfo>& infos);
    bool addMedia(const rtc::scoped_refptr<webrtc::RtpTransceiverInterface>& transceiver);
    bool addMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    bool removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    bool addDataChannel(rtc::scoped_refptr<DataChannel> channel);
    void reset();
    size_t count() const;
    std::shared_ptr<RemoteParticipant> at(size_t index) const;
    std::shared_ptr<RemoteParticipant> at(const std::string& sid) const;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    // non thread-safe to [_participants]
    std::vector<ParticipantInfo> infos() const;
    bool addMedia(const std::string& sid,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    void addMedia(const std::shared_ptr<RemoteParticipantImpl>& participant,
                  TrackType type, const std::string& sid,
                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) const;
    bool addToOrphans(std::string sid,
                      const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    // service methods, non thread-safe to [_participants]
    void addParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant);
    void removeParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant);
    void removeParticipant(const std::string& sid);
    void clearParticipants();
    std::optional<size_t> findBySid(const std::string& sid) const;
private:
    TrackManager* const _trackManager;
    RemoteParticipantsListener* const _listener;
    Bricks::SafeObj<OrphanedTracks> _orphans;
    Bricks::SafeObj<Participants> _participants;
};

} // namespace LiveKitCpp
#endif
