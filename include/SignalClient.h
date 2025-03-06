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
#include "LiveKitClientExport.h"
#include "Logger.h"
#include "State.h"
#include <memory>
#include <string>

class Logger;

namespace LiveKitCpp
{

class CommandSender;
class MemoryBlock;
class SignalServerListener;
class SignalTransportListener;
class ResponseReceiver;
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

class LIVEKIT_CLIENT_API SignalClient : protected Logger
{
    class Impl;
protected:
    enum class ChangeTransportStateResult
    {
        Changed,
        NotChanged,
        Rejected
    };
public:
    SignalClient(CommandSender* commandSender, Logger* logger = nullptr);
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
    static std::string_view defaultLogCategory();
    ChangeTransportStateResult changeTransportState(State state);
    void notifyAboutTransportError(const std::string& error);
    void handleServerProtobufMessage(const void* message, size_t len);
    // impl. of Logger
    bool canLog(LoggingSeverity severity) const final;
    void log(LoggingSeverity severity, std::string_view message,
             std::string_view category = {}) final;
private:
    const std::unique_ptr<Impl> _impl;
    // for handling of incoming messages from the LiveKit SFU
    const std::unique_ptr<ResponseReceiver> _responseReceiver;
    // for sending requests to the LiveKit SFU
    const std::unique_ptr<const RequestSender> _requestSender;
};

} // namespace LiveKitCpp
