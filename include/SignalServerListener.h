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
#include "LiveKitClientExport.h"
#include <string>

namespace LiveKitCpp
{

// these below structs are defined in /include/rtc subfolder
struct ConnectionQualityUpdate;
struct JoinResponse;
struct SessionDescription;
struct TrickleRequest;
struct ParticipantUpdate;
struct TrackPublishedResponse;
struct TrackUnpublishedResponse;
struct LeaveRequest;
struct MuteTrackRequest;
struct SpeakersChanged;
struct RoomUpdate;
struct StreamStateUpdate;
struct SubscribedQualityUpdate;
struct ReconnectResponse;
struct TrackSubscribed;
struct RequestResponse;
struct SubscriptionResponse;
struct SubscriptionPermissionUpdate;
struct Pong;

class LIVEKIT_CLIENT_API SignalServerListener
{
public:
    virtual void onJoin(const JoinResponse& /*response*/) {}
    virtual void onOffer(const SessionDescription& /*sdp*/) {}
    virtual void onAnswer(const SessionDescription& /*sdp*/) {}
    virtual void onTrickle(const TrickleRequest& /*request*/) {}
    virtual void onUpdate(const ParticipantUpdate& /*update*/) {}
    virtual void onTrackPublished(const TrackPublishedResponse& /*published*/) {}
    virtual void onTrackUnpublished(const TrackUnpublishedResponse& /*unpublished*/) {}
    virtual void onLeave(const LeaveRequest& /*leave*/) {}
    virtual void onMute(const MuteTrackRequest& /*mute*/) {}
    virtual void onSpeakersChanged(const SpeakersChanged& /*changed*/) {}
    virtual void onRoomUpdate(const RoomUpdate& /*update*/) {}
    virtual void onConnectionQuality(const ConnectionQualityUpdate& /*update*/) {}
    virtual void onStreamStateUpdate(const StreamStateUpdate& /*update*/) {}
    virtual void onSubscribedQualityUpdate(const SubscribedQualityUpdate& /*update*/) {}
    virtual void onSubscriptionPermission(const SubscriptionPermissionUpdate& /*update*/) {}
    virtual void onRefreshToken(const std::string& /*authToken*/) {}
    virtual void onReconnect(const ReconnectResponse& /*response*/) {}
    virtual void onTrackSubscribed(const TrackSubscribed& /*subscribed*/) {}
    virtual void onRequestResponse(const RequestResponse& /*response*/) {}
    virtual void onSubscriptionResponse(const SubscriptionResponse& /*response*/) {}
    virtual void onPong(const Pong& /*pong*/) {}
    // error handling
    virtual void onSignalParseError() {}
};

} // namespace LiveKitCpp
