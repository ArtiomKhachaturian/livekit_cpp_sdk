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
#include "Blob.h"

using Request = livekit::SignalRequest;

namespace {

class VectorBlob : public Bricks::Blob
{
public:
    VectorBlob(const std::vector<uint8_t>& data);
    // impl. of Bricks::Blob
    size_t size() const noexcept final { return _data.size(); }
    const uint8_t* data() const noexcept final { return _data.data(); }
private:
    const std::vector<uint8_t>& _data;
};

}

namespace LiveKitCpp
{

RequestInterceptor::RequestInterceptor(CommandSender* commandSender, Bricks::Logger* logger)
    : _commandSender(commandSender)
    , _marshaller(logger)
{
}

bool RequestInterceptor::offer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_offer, sdp);
}

bool RequestInterceptor::answer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_answer, sdp);
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

template <class TSetMethod, class TObject>
bool RequestInterceptor::send(const TSetMethod& setMethod, const TObject& object) const
{
    if (canSend()) {
        Request request;
        if (const auto target = (request.*setMethod)()) {
            *target = _marshaller.map(object);
            const auto bytes = _marshaller.toBytes(request);
            if (!bytes.empty()) {
                return _commandSender->sendBinary(VectorBlob(bytes));
            }
        }
    }
    return false;
}

} // namespace LiveKitCpp

namespace {

VectorBlob::VectorBlob(const std::vector<uint8_t>& data)
    : _data(data)
{
}

}
