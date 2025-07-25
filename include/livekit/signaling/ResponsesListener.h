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
#pragma once // SignalServerListener.h
#include <string>
#include "livekit/signaling/sfu/ConnectionQualityUpdate.h"
#include "livekit/signaling/sfu/JoinResponse.h"
#include "livekit/signaling/sfu/SessionDescription.h"
#include "livekit/signaling/sfu/TrickleRequest.h"
#include "livekit/signaling/sfu/ParticipantUpdate.h"
#include "livekit/signaling/sfu/TrackPublishedResponse.h"
#include "livekit/signaling/sfu/TrackUnpublishedResponse.h"
#include "livekit/signaling/sfu/LeaveRequest.h"
#include "livekit/signaling/sfu/MuteTrackRequest.h"
#include "livekit/signaling/sfu/SpeakersChanged.h"
#include "livekit/signaling/sfu/RoomUpdate.h"
#include "livekit/signaling/sfu/StreamStateUpdate.h"
#include "livekit/signaling/sfu/SubscribedQualityUpdate.h"
#include "livekit/signaling/sfu/ReconnectResponse.h"
#include "livekit/signaling/sfu/TrackSubscribed.h"
#include "livekit/signaling/sfu/RequestResponse.h"
#include "livekit/signaling/sfu/SubscriptionResponse.h"
#include "livekit/signaling/sfu/SubscriptionPermissionUpdate.h"
#include "livekit/signaling/sfu/Pong.h"
#include "livekit/signaling/sfu/DataPacket.h"
#include "livekit/signaling/sfu/RoomMovedResponse.h"

namespace LiveKitCpp
{

class ResponsesListener
{
public:
    virtual void onJoin(JoinResponse /*response*/) {}
    virtual void onOffer(SessionDescription /*sdp*/) {}
    virtual void onAnswer(SessionDescription /*sdp*/) {}
    virtual void onTrickle(TrickleRequest /*request*/) {}
    virtual void onUpdate(ParticipantUpdate /*update*/) {}
    virtual void onTrackPublished(TrackPublishedResponse /*published*/) {}
    virtual void onTrackUnpublished(TrackUnpublishedResponse /*unpublished*/) {}
    virtual void onLeave(LeaveRequest /*leave*/) {}
    virtual void onMute(MuteTrackRequest /*mute*/) {}
    virtual void onSpeakersChanged(SpeakersChanged /*changed*/) {}
    virtual void onRoomUpdate(RoomUpdate /*update*/) {}
    virtual void onConnectionQuality(ConnectionQualityUpdate /*update*/) {}
    virtual void onStreamStateUpdate(StreamStateUpdate /*update*/) {}
    virtual void onSubscribedQualityUpdate(SubscribedQualityUpdate /*update*/) {}
    virtual void onSubscriptionPermission(SubscriptionPermissionUpdate /*update*/) {}
    virtual void onRefreshToken(std::string /*authToken*/) {}
    virtual void onReconnect(ReconnectResponse /*response*/) {}
    virtual void onTrackSubscribed(TrackSubscribed /*subscribed*/) {}
    virtual void onRequestResponse(RequestResponse /*response*/) {}
    virtual void onSubscriptionResponse(SubscriptionResponse /*response*/) {}
    virtual void onPong(Pong /*pong*/) {}
    virtual void onDataPacket(DataPacket /*packet*/) {}
    virtual void onRoomMovedResponse(RoomMovedResponse /*response*/) {}
    // error handling
    virtual void onResponseParseError(std::string /*details*/ = {}) {}
protected:
    virtual ~ResponsesListener() = default;
};

} // namespace LiveKitCpp
