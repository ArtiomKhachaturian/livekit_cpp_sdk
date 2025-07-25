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
#include "Loggable.h"
#include "livekit/signaling/LiveKitSignalingExport.h"
#include "livekit/signaling/sfu/SessionDescription.h"
#include "livekit/signaling/sfu/TrickleRequest.h"
#include "livekit/signaling/sfu/AddTrackRequest.h"
#include "livekit/signaling/sfu/MuteTrackRequest.h"
#include "livekit/signaling/sfu/UpdateSubscription.h"
#include "livekit/signaling/sfu/UpdateTrackSettings.h"
#include "livekit/signaling/sfu/LeaveRequest.h"
#include "livekit/signaling/sfu/UpdateVideoLayers.h"
#include "livekit/signaling/sfu/SubscriptionPermission.h"
#include "livekit/signaling/sfu/SyncState.h"
#include "livekit/signaling/sfu/SimulateScenario.h"
#include "livekit/signaling/sfu/UpdateParticipantMetadata.h"
#include "livekit/signaling/sfu/Ping.h"
#include "livekit/signaling/sfu/UpdateLocalAudioTrack.h"
#include "livekit/signaling/sfu/UpdateLocalVideoTrack.h"
#include "livekit/signaling/sfu/DataPacket.h"
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
class ResponsesListener;
class ResponseReceiver;
class RequestSender;

class LIVEKIT_SIGNALING_API SignalClient : protected Bricks::LoggableR<>
{
public:
    SignalClient(CommandSender* commandSender, Bricks::Logger* logger = nullptr);
    virtual ~SignalClient();
    void setServerListener(ResponsesListener* listener = nullptr);
    void parseProtobuBlob(const Bricks::Blob& message);
    void parseProtobufData(const void* data, size_t dataLen);
    void notifyAboutError(std::string details = {});
    // requests sending
    bool sendOffer(SessionDescription sdp) const;
    bool sendAnswer(SessionDescription sdp) const;
    bool sendTrickle(TrickleRequest request) const;
    bool sendAddTrack(AddTrackRequest request) const;
    bool sendMuteTrack(MuteTrackRequest request) const;
    bool sendSubscription(UpdateSubscription update) const;
    bool sendTrackSettings(UpdateTrackSettings update) const;
    bool sendLeave(LeaveRequest request) const;
    bool sendUpdateVideoLayers(UpdateVideoLayers update) const;
    bool sendSubscriptionPermission(SubscriptionPermission permission) const;
    bool sendSyncState(SyncState state) const;
    bool sendSimulate(SimulateScenario scenario) const;
    bool sendUpdateMetadata(UpdateParticipantMetadata data) const;
    bool sendPingReq(Ping ping) const;
    bool sendUpdateAudioTrack(UpdateLocalAudioTrack track) const;
    bool sendUpdateVideoTrack(UpdateLocalVideoTrack track) const;
    bool sendDataPacket(DataPacket packet) const;
protected:
    // impl. of Bricks::LoggableR<>
    std::string_view logCategory() const override;
private:
    // for handling of incoming messages from the LiveKit SFU
    const std::unique_ptr<ResponseReceiver> _responseReceiver;
    // for sending requests to the LiveKit SFU
    const std::unique_ptr<const RequestSender> _requestSender;
};

} // namespace LiveKitCpp
