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
#pragma once // SignalHandler.h
#include "Listeners.h"
#include "livekit_rtc.pb.h"
#include <memory>

namespace LiveKitCpp
{

class SignalServerListener;

class SignalHandler
{
public:
    SignalHandler(uint64_t signalClientId);
    void parseBinary(const void* data, size_t dataLen);
    void addListener(SignalServerListener* listener);
    void removeListener(SignalServerListener* listener);
private:
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    template <class Method, class TLiveKitType>
    void signal(const Method& method, const TLiveKitType& sig) const;
    void handle(const livekit::JoinResponse& response) const;
    void handle(const livekit::SessionDescription& desc, bool offer) const;
    void handle(const livekit::TrickleRequest& request) const;
    void handle(const livekit::ParticipantUpdate& update) const;
    void handle(const livekit::TrackPublishedResponse& response) const;
    void handle(const livekit::LeaveRequest& request) const;
    void handle(const livekit::MuteTrackRequest& request) const;
    void handle(const livekit::SpeakersChanged& changed) const;
    void handle(const livekit::RoomUpdate& update) const;
    void handle(const livekit::ConnectionQualityUpdate& update) const;
    void handle(const livekit::StreamStateUpdate& update) const;
    void handle(const livekit::SubscribedQualityUpdate& update) const;
    void handle(const livekit::SubscriptionPermissionUpdate& update) const;
    void handle(const livekit::TrackUnpublishedResponse& response) const;
    void handle(const livekit::ReconnectResponse& response) const;
    void handle(const livekit::SubscriptionResponse& response) const;
    void handle(const livekit::RequestResponse& response) const;
    void handle(const livekit::TrackSubscribed& subscribed) const;
    void handle(const livekit::Pong& pong) const;
private:
    const uint64_t _signalClientId;
    Listeners<SignalServerListener*> _listeners;
};

} // namespace LiveKitCpp
