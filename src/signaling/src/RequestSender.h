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
#include "ProtoMarshaller.h"
#include <memory>
#include <vector>

namespace google::protobuf {
class MessageLite;
}

namespace LiveKitCpp
{

class CommandSender;

class RequestSender : private Bricks::LoggableR<>
{
public:
    RequestSender(CommandSender* commandSender, Bricks::Logger* logger = nullptr);
    // all requests are defined in 'SignalRequest':
    // https://github.com/livekit/protocol/blob/main/protobufs/livekit_rtc.proto#L24
    bool offer(SessionDescription sdp) const;
    bool answer(SessionDescription sdp) const;
    bool trickle(TrickleRequest request) const;
    bool addTrack(AddTrackRequest request) const;
    bool muteTrack(MuteTrackRequest request) const;
    bool subscription(UpdateSubscription update) const;
    bool trackSettings(UpdateTrackSettings update) const;
    bool leave(LeaveRequest request) const;
    bool updateVideoLayers(UpdateVideoLayers update) const;
    bool subscriptionPermission(SubscriptionPermission permission) const;
    bool syncState(SyncState state) const;
    bool simulate(SimulateScenario scenario) const;
    bool updateMetadata(UpdateParticipantMetadata data) const;
    bool pingReq(Ping ping) const;
    bool updateAudioTrack(UpdateLocalAudioTrack track) const;
    bool updateVideoTrack(UpdateLocalVideoTrack track) const;
    bool dataPacket(DataPacket packet) const;
protected:
    // overrides of Bricks::LoggableR
    std::string_view logCategory() const final;
private:
    bool canSend() const;
    template <class TSetMethod, class TObject>
    bool send(const TSetMethod& setMethod, TObject object, const std::string& typeName = {}) const;
    template <class TProtoObject>
    bool send(const TProtoObject& object, const std::string& typeName = {}) const;
private:
    CommandSender* const _commandSender;
    const ProtoMarshaller _marshaller;
};

} // namespace LiveKitCpp
