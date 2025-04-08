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
#include "RequestInterceptor.h"
#include "CommandSender.h"
#include "MarshalledTypesFwd.h"
#include "ProtoUtils.h"
#include "Blob.h"

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

RequestInterceptor::RequestInterceptor(CommandSender* commandSender, Bricks::Logger* logger)
    : Bricks::LoggableR<>(logger)
    , _commandSender(commandSender)
    , _marshaller(logger)
{
}

bool RequestInterceptor::offer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_offer, sdp,
                marshalledTypeName<SessionDescription>() + "/offer");
}

bool RequestInterceptor::answer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_answer, sdp,
                marshalledTypeName<SessionDescription>() + "/answer");
}

bool RequestInterceptor::trickle(const TrickleRequest& request) const
{
    return send(&Request::mutable_trickle, request);
}

bool RequestInterceptor::addTrack(const AddTrackRequest& request) const
{
    return send(&Request::mutable_add_track, request);
}

bool RequestInterceptor::muteTrack(const MuteTrackRequest& request) const
{
    return send(&Request::mutable_mute, request);
}

bool RequestInterceptor::subscription(const UpdateSubscription& update) const
{
    return send(&Request::mutable_subscription, update);
}

bool RequestInterceptor::trackSettings(const UpdateTrackSettings& update) const
{
    return send(&Request::mutable_track_setting, update);
}

bool RequestInterceptor::leave(const LeaveRequest& request) const
{
    return send(&Request::mutable_leave, request);
}

bool RequestInterceptor::updateVideoLayers(const UpdateVideoLayers& update) const
{
    return send(&Request::mutable_update_layers, update);
}

bool RequestInterceptor::subscriptionPermission(const SubscriptionPermission& permission) const
{
    return send(&Request::mutable_subscription_permission, permission);
}

bool RequestInterceptor::syncState(const SyncState& state) const
{
    return send(&Request::mutable_sync_state, state);
}

bool RequestInterceptor::simulate(const SimulateScenario& scenario) const
{
    return send(&Request::mutable_simulate, scenario);
}

bool RequestInterceptor::updateMetadata(const UpdateParticipantMetadata& data) const
{
    return send(&Request::mutable_update_metadata, data);
}

bool RequestInterceptor::pingReq(const Ping& ping) const
{
    return send(&Request::mutable_ping_req, ping);
}

bool RequestInterceptor::updateAudioTrack(const UpdateLocalAudioTrack& track) const
{
    return send(&Request::mutable_update_audio_track, track);
}

bool RequestInterceptor::updateVideoTrack(const UpdateLocalVideoTrack& track) const
{
    return send(&Request::mutable_update_video_track, track);
}

bool RequestInterceptor::canSend() const
{
    return nullptr != _commandSender;
}

std::string_view RequestInterceptor::logCategory() const
{
    static const std::string_view category("request_interceptor");
    return category;
}

template <class TSetMethod, class TObject>
bool RequestInterceptor::send(const TSetMethod& setMethod, const TObject& object,
                              std::string typeName) const
{
    bool ok = false;
    if (canSend()) {
        Request request;
        if (typeName.empty()) {
            typeName = marshalledTypeName<TObject>();
        }
        if (const auto target = (request.*setMethod)()) {
            *target = _marshaller.map(object);
            const auto bytes = protoToBytes(request, logger(), logCategory());
            if (!bytes.empty()) {
                ok = _commandSender->sendBinary(VectorBlob(bytes));
                if (ok) {
                    logVerbose("sending '" + typeName + "' signal to server");
                }
                else {
                    logError("send of '" + typeName + "' signal has been failed");
                }
            }
            else {
                logError("failed to serialize of '" + typeName + "' into a bytes array");
            }
        }
        else {
            logError("proto method not available for set of '" + typeName + "'");
        }
    }
    return ok;
}

} // namespace LiveKitCpp

namespace {

VectorBlob::VectorBlob(const std::vector<uint8_t>& data)
    : _data(data)
{
}

}
