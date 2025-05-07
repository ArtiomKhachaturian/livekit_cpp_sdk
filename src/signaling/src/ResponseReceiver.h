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
#pragma once // ResponseReceiver.h
#include "Listener.h"
#include "ProtoMarshaller.h"
#include "livekit_rtc.pb.h"
#include <memory>
#include <optional>

namespace LiveKitCpp
{

class ResponsesListener;

class ResponseReceiver : private Bricks::LoggableR<>
{
public:
    ResponseReceiver(Bricks::Logger* logger = nullptr);
    void parseBinary(const void* data, size_t dataLen);
    void setListener(ResponsesListener* listener = nullptr) { _listener = listener; }
    void notifyAboutError(std::string details = {});
protected:
    // overrides of Bricks::LoggableR
    std::string_view logCategory() const final;
private:
    std::optional<livekit::SignalResponse> parseResponse(const void* data, size_t dataLen) const;
    std::optional<livekit::DataPacket> parseDataPacket(const void* data, size_t dataLen) const;
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    template <class Method, class TLiveKitType>
    void signal(const Method& method, TLiveKitType sig, std::string typeName = {}) const;
    // all responses are defined in 'SignalResponse':
    // https://github.com/livekit/protocol/blob/main/protobufs/livekit_rtc.proto#L61
    void handle(livekit::JoinResponse response) const;
    void handle(livekit::SessionDescription desc, bool offer) const;
    void handle(livekit::TrickleRequest request) const;
    void handle(livekit::ParticipantUpdate update) const;
    void handle(livekit::TrackPublishedResponse response) const;
    void handle(livekit::LeaveRequest request) const;
    void handle(livekit::MuteTrackRequest request) const;
    void handle(livekit::SpeakersChanged changed) const;
    void handle(livekit::RoomUpdate update) const;
    void handle(livekit::ConnectionQualityUpdate update) const;
    void handle(livekit::StreamStateUpdate update) const;
    void handle(livekit::SubscribedQualityUpdate update) const;
    void handle(livekit::SubscriptionPermissionUpdate update) const;
    void handle(livekit::TrackUnpublishedResponse response) const;
    void handle(livekit::ReconnectResponse response) const;
    void handle(livekit::SubscriptionResponse response) const;
    void handle(livekit::RequestResponse response) const;
    void handle(livekit::TrackSubscribed subscribed) const;
    void handle(livekit::Pong pong) const;
    void handle(livekit::DataPacket packet) const;
    void handle(livekit::RoomMovedResponse response) const;
private:
    const ProtoMarshaller _marshaller;
    Bricks::Listener<ResponsesListener*> _listener;
};

} // namespace LiveKitCpp
