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
#include "RequestSender.h"
#include "MarshalledTypesFwd.h"
#include "ProtoUtils.h"
#include "Blob.h"
#include "livekit/signaling/CommandSender.h"

using Request = livekit::SignalRequest;

namespace {

class VectorBlob : public Bricks::Blob
{
public:
    VectorBlob(const std::vector<uint8_t>& data);
    // impl. of Bricks::Blob
    size_t size() const final { return _data.size(); }
    const uint8_t* data() const final { return _data.data(); }
private:
    const std::vector<uint8_t>& _data;
};

template <typename T>
inline std::string requestTypeName() { static_assert(false, "type name not evaluated"); }

}

namespace LiveKitCpp
{

MARSHALLED_TYPE_NAME_DECL(SessionDescription)
MARSHALLED_TYPE_NAME_DECL(TrickleRequest)
MARSHALLED_TYPE_NAME_DECL(AddTrackRequest)
MARSHALLED_TYPE_NAME_DECL(MuteTrackRequest)
MARSHALLED_TYPE_NAME_DECL(UpdateSubscription)
MARSHALLED_TYPE_NAME_DECL(UpdateTrackSettings)
MARSHALLED_TYPE_NAME_DECL(LeaveRequest)
MARSHALLED_TYPE_NAME_DECL(UpdateVideoLayers)
MARSHALLED_TYPE_NAME_DECL(SubscriptionPermission)
MARSHALLED_TYPE_NAME_DECL(SyncState)
MARSHALLED_TYPE_NAME_DECL(SimulateScenario)
MARSHALLED_TYPE_NAME_DECL(UpdateParticipantMetadata)
MARSHALLED_TYPE_NAME_DECL(Ping)
MARSHALLED_TYPE_NAME_DECL(UpdateLocalAudioTrack)
MARSHALLED_TYPE_NAME_DECL(UpdateLocalVideoTrack)
MARSHALLED_TYPE_NAME_DECL(DataPacket)

template <typename T>
inline std::string detectTypename(const std::string& typeName) {
    if (typeName.empty()) {
        return marshalledTypeName<T>();
    }
    return typeName;
}

RequestSender::RequestSender(CommandSender* commandSender, Bricks::Logger* logger)
    : Bricks::LoggableR<>(logger)
    , _commandSender(commandSender)
    , _marshaller(logger)
{
}

bool RequestSender::offer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_offer, sdp,
                marshalledTypeName<SessionDescription>() + "/offer");
}

bool RequestSender::answer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_answer, sdp,
                marshalledTypeName<SessionDescription>() + "/answer");
}

bool RequestSender::trickle(const TrickleRequest& request) const
{
    return send(&Request::mutable_trickle, request);
}

bool RequestSender::addTrack(const AddTrackRequest& request) const
{
    return send(&Request::mutable_add_track, request);
}

bool RequestSender::muteTrack(const MuteTrackRequest& request) const
{
    return send(&Request::mutable_mute, request);
}

bool RequestSender::subscription(const UpdateSubscription& update) const
{
    return send(&Request::mutable_subscription, update);
}

bool RequestSender::trackSettings(const UpdateTrackSettings& update) const
{
    return send(&Request::mutable_track_setting, update);
}

bool RequestSender::leave(const LeaveRequest& request) const
{
    return send(&Request::mutable_leave, request);
}

bool RequestSender::updateVideoLayers(const UpdateVideoLayers& update) const
{
    return send(&Request::mutable_update_layers, update);
}

bool RequestSender::subscriptionPermission(const SubscriptionPermission& permission) const
{
    return send(&Request::mutable_subscription_permission, permission);
}

bool RequestSender::syncState(const SyncState& state) const
{
    return send(&Request::mutable_sync_state, state);
}

bool RequestSender::simulate(const SimulateScenario& scenario) const
{
    return send(&Request::mutable_simulate, scenario);
}

bool RequestSender::updateMetadata(const UpdateParticipantMetadata& data) const
{
    return send(&Request::mutable_update_metadata, data);
}

bool RequestSender::pingReq(const Ping& ping) const
{
    return send(&Request::mutable_ping_req, ping);
}

bool RequestSender::updateAudioTrack(const UpdateLocalAudioTrack& track) const
{
    return send(&Request::mutable_update_audio_track, track);
}

bool RequestSender::updateVideoTrack(const UpdateLocalVideoTrack& track) const
{
    return send(&Request::mutable_update_video_track, track);
}

bool RequestSender::dataPacket(const DataPacket& packet) const
{
    return send(_marshaller.map(packet));
}

bool RequestSender::canSend() const
{
    return nullptr != _commandSender;
}

std::string_view RequestSender::logCategory() const
{
    static const std::string_view category("request_interceptor");
    return category;
}

template <class TSetMethod, class TObject>
bool RequestSender::send(const TSetMethod& setMethod, const TObject& object,
                         const std::string& typeName) const
{
    bool ok = false;
    if (canSend()) {
        Request request;
        if (const auto target = (request.*setMethod)()) {
            *target = _marshaller.map(object);
            ok = send(request, detectTypename<TObject>(typeName));
        }
        else {
            logError("proto method not available for set of '" +
                     detectTypename<TObject>(typeName) + "'");
        }
    }
    return ok;
}

template <class TProtoObject>
bool RequestSender::send(const TProtoObject& object, const std::string& typeName) const
{
    const auto bytes = protoToBytes(object, logger(), logCategory());
    if (!bytes.empty()) {
        const auto ok = _commandSender->sendBinary(VectorBlob(bytes));
        if (ok) {
            logVerbose("sending '" + typeName + "' to server");
        }
        else {
            logError("send of '" + typeName + "' signal has been failed");
        }
    }
    else {
        logError("failed to serialize of '" + typeName + "' into a bytes array");
    }
}

} // namespace LiveKitCpp

namespace {

VectorBlob::VectorBlob(const std::vector<uint8_t>& data)
    : _data(data)
{
}

}
