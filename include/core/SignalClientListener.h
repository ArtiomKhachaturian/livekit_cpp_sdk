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
#pragma once // SignalClientListener.h
#include <chrono>
#include <string>

namespace LiveKitCpp
{

enum class State;
// these structs are defined in /include/rtc subfolder
struct ConnectionQualityUpdate;
struct JoinResponse;
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

class SignalClientListener
{
public:
    virtual void onTransportStateChanged(uint64_t /*signalClientId*/, State /*state*/) {}
    virtual void onServerResponseParseError(uint64_t /*signalClientId*/) {}
    virtual void onJoin(uint64_t /*signalClientId*/,
                        const JoinResponse& /*response*/) {}
    virtual void onOffer(uint64_t /*signalClientId*/, const std::string& /*type*/,
                         const std::string& /*sdp*/) {}
    virtual void onAnswer(uint64_t /*signalClientId*/, const std::string& /*type*/,
                          const std::string& /*sdp*/) {}
    virtual void onTrickle(uint64_t /*signalClientId*/,
                           const TrickleRequest& /*request*/) {}
    virtual void onParticipantUpdate(uint64_t /*signalClientId*/,
                                     const ParticipantUpdate& /*update*/) {}
    virtual void onTrackPublished(uint64_t /*signalClientId*/,
                                  const TrackPublishedResponse& /*published*/) {}
    virtual void onTrackUnpublished(uint64_t /*signalClientId*/,
                                    const TrackUnpublishedResponse& /*unpublished*/) {}
    virtual void onLeave(uint64_t /*signalClientId*/,
                         const LeaveRequest& /*leave*/) {}
    virtual void onMuteTrack(uint64_t /*signalClientId*/,
                             const MuteTrackRequest& /*mute*/) {}
    virtual void onSpeakersChanged(uint64_t /*signalClientId*/,
                                   const SpeakersChanged& /*changed*/) {}
    virtual void onRoomUpdate(uint64_t /*signalClientId*/,
                              const RoomUpdate& /*update*/) {}
    virtual void onConnectionQualityUpdate(uint64_t /*signalClientId*/,
                                           const ConnectionQualityUpdate& /*update*/) {}
    virtual void onStreamStateUpdate(uint64_t /*signalClientId*/,
                                     const StreamStateUpdate& /*update*/) {}
    virtual void onSubscribedQualityUpdate(uint64_t /*signalClientId*/,
                                           const SubscribedQualityUpdate& /*update*/) {}
    virtual void onRefreshToken(uint64_t /*signalClientId*/,
                                const std::string& /*authToken*/) {}
    virtual void onReconnect(uint64_t /*signalClientId*/,
                             const ReconnectResponse& /*response*/) {}
    virtual void onPong(uint64_t /*signalClientId*/,
                        const std::chrono::milliseconds& /*timestamp*/,
                        const std::chrono::milliseconds& /*lastPingTimestamp*/ = {}) {}
protected:
    virtual ~SignalClientListener() = default;
};

} // namespace LiveKitCpp
