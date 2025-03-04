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
#pragma once // RequestSender.h
#include <memory>
#include <vector>

namespace google::protobuf {
class MessageLite;
}

namespace LiveKitCpp
{

class CommandSender;
class MemoryBlock;
struct SessionDescription;
struct TrickleRequest;
struct AddTrackRequest;
struct MuteTrackRequest;
struct UpdateSubscription;
struct UpdateTrackSettings;
struct LeaveRequest;
struct UpdateVideoLayers;
struct SubscriptionPermission;
struct SyncState;
struct SimulateScenario;
struct UpdateParticipantMetadata;
struct Ping;
struct UpdateLocalAudioTrack;
struct UpdateLocalVideoTrack;

class RequestSender
{
public:
    RequestSender(CommandSender* commandSender);
    bool offer(const SessionDescription& sdp) const;
    bool answer(const SessionDescription& sdp) const;
    bool trickle(const TrickleRequest& request) const;
    bool addTrack(const AddTrackRequest& request) const;
    bool muteTrack(const MuteTrackRequest& request) const;
    bool subscription(const UpdateSubscription& update) const;
    bool trackSettings(const UpdateTrackSettings& update) const;
    bool leave(const LeaveRequest& request) const;
    bool updateVideoLayers(const UpdateVideoLayers& update) const;
    bool subscriptionPermission(const SubscriptionPermission& permission) const;
    bool syncState(const SyncState& state) const;
    bool simulate(const SimulateScenario& scenario) const;
    bool updateMetadata(const UpdateParticipantMetadata& data) const;
    bool pingReq(const Ping& ping) const;
    bool updateAudioTrack(const UpdateLocalAudioTrack& track) const;
    bool updateVideoTrack(const UpdateLocalVideoTrack& track) const;
private:
    bool canSend() const;
    template <class TSetMethod, class TObject>
    bool send(const TSetMethod& setMethod, const TObject& object) const;
    static std::vector<uint8_t> toBytes(const google::protobuf::MessageLite& proto);
    static std::shared_ptr<MemoryBlock> toMemBlock(const google::protobuf::MessageLite& proto);
private:
    CommandSender* const _commandSender;
};

} // namespace LiveKitCpp
