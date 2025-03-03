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
#include <memory>
#include "State.h"

namespace LiveKitCpp
{

class CommandSender;
class MemoryBlock;
class SignalServerListener;
class SignalTransportListener;
class ResponseReceiver;
class RequestSender;
struct SessionDescription;
struct TrickleRequest;
struct AddTrackRequest;
struct MuteTrackRequest;
struct UpdateSubscription;
struct UpdateTrackSettings;

class SignalClient
{
    class Impl;
public:
    SignalClient(CommandSender* commandSender);
    virtual ~SignalClient();
    void addTransportListener(SignalTransportListener* listener);
    void addServerListener(SignalServerListener* listener);
    void removeTransportListener(SignalTransportListener* listener);
    void removeServerListener(SignalServerListener* listener);
    virtual bool connect();
    virtual void disconnect();
    State transportState() const noexcept;
    uint64_t id() const noexcept { return reinterpret_cast<uint64_t>(this); }
    // requests sending
    bool sendOffer(const SessionDescription& sdp) const;
    bool sendAnswer(const SessionDescription& sdp) const;
    bool sendTrickle(const TrickleRequest& request) const;
    bool sendAddTrack(const AddTrackRequest& request) const;
    bool sendMuteTrack(const MuteTrackRequest& request) const;
    bool sendSubscription(const UpdateSubscription& update) const;
    bool sendTrackSettings(const UpdateTrackSettings& update) const;
protected:
    bool changeTransportState(State state);
    void notifyAboutTransportError(const std::string& error);
    void handleServerProtobufMessage(const std::shared_ptr<const MemoryBlock>& message);
    void handleServerProtobufMessage(const void* message, size_t len);
private:
    const std::unique_ptr<Impl> _impl;
    // for handling of incoming messages from the LiveKit SFU
    const std::unique_ptr<ResponseReceiver> _responseReceiver;
    // for sending requests to the LiveKit SFU
    const std::unique_ptr<RequestSender> _requestSender;
};

} // namespace LiveKitCpp
