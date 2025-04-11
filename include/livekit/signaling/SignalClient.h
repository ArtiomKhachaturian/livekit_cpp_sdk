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
#pragma once // SignalClient.h
#include "livekit/signaling/LiveKitSignalingExport.h"
#include "Loggable.h"
#include <memory>
#include <string>

namespace Bricks {
class Logger;
class Blob;
}

namespace LiveKitCpp
{

class CommandSender;
class MemoryBlock;
class SignalServerListener;
class ResponseInterceptor;
class RequestSender;
// these below structs are defined in /include/rtc subfolder
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

class LIVEKIT_SIGNALING_API SignalClient : protected Bricks::LoggableR<>
{
public:
    SignalClient(CommandSender* commandSender, Bricks::Logger* logger = nullptr);
    virtual ~SignalClient();
    void setServerListener(SignalServerListener* listener = nullptr);
    // requests sending
    bool sendOffer(const SessionDescription& sdp) const;
    bool sendAnswer(const SessionDescription& sdp) const;
    bool sendTrickle(const TrickleRequest& request) const;
    bool sendAddTrack(const AddTrackRequest& request) const;
    bool sendMuteTrack(const MuteTrackRequest& request) const;
    bool sendSubscription(const UpdateSubscription& update) const;
    bool sendTrackSettings(const UpdateTrackSettings& update) const;
    bool sendLeave(const LeaveRequest& request) const;
    bool sendUpdateVideoLayers(const UpdateVideoLayers& update) const;
    bool sendSubscriptionPermission(const SubscriptionPermission& permission) const;
    bool sendSyncState(const SyncState& state) const;
    bool sendSimulate(const SimulateScenario& scenario) const;
    bool sendUpdateMetadata(const UpdateParticipantMetadata& data) const;
    bool sendPingReq(const Ping& ping) const;
    bool sendUpdateAudioTrack(const UpdateLocalAudioTrack& track) const;
    bool sendUpdateVideoTrack(const UpdateLocalVideoTrack& track) const;
protected:
    void handleServerProtobufMessage(const Bricks::Blob& message);
    // impl. of Bricks::LoggableR<>
    std::string_view logCategory() const override;
private:
    // for handling of incoming messages from the LiveKit SFU
    const std::unique_ptr<ResponseInterceptor> _responseReceiver;
    // for sending requests to the LiveKit SFU
    const std::unique_ptr<const RequestSender> _requestSender;
};

} // namespace LiveKitCpp
