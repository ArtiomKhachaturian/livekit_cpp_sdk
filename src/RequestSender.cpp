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
#include "CommandSender.h"
#include "Blob.h"

using Request = livekit::SignalRequest;

namespace {

class VectorBlob : public Bricks::Blob
{
public:
    VectorBlob(std::vector<uint8_t> data);
    // impl. of Bricks::Blob
    size_t size() const noexcept final { return _data.size(); }
    const uint8_t* data() const noexcept final { return _data.data(); }
private:
    const std::vector<uint8_t> _data;
};

}

namespace LiveKitCpp
{

RequestSender::RequestSender(CommandSender* commandSender, Bricks::Logger* logger)
    : _commandSender(commandSender)
    , _signals(logger)
{
}

bool RequestSender::offer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_offer, sdp);
}

bool RequestSender::answer(const SessionDescription& sdp) const
{
    return send(&Request::mutable_answer, sdp);
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

bool RequestSender::canSend() const
{
    return nullptr != _commandSender;
}

template <class TSetMethod, class TObject>
bool RequestSender::send(const TSetMethod& setMethod, const TObject& object) const
{
    if (canSend()) {
        Request request;
        if (const auto target = (request.*setMethod)()) {
            *target = _signals.map(object);
            if (const auto blob = toBlob(request)) {
                return _commandSender->sendBinary(blob);
            }
        }
    }
    return false;
}

std::vector<uint8_t> RequestSender::toBytes(const google::protobuf::MessageLite& proto)
{
    if (const auto size = proto.ByteSizeLong()) {
        std::vector<uint8_t> buffer(size);
        if (proto.SerializeToArray(buffer.data(), int(size))) {
            return buffer;
        }
    }
    return {};
}

std::shared_ptr<Bricks::Blob> RequestSender::toBlob(const google::protobuf::MessageLite& proto)
{
    auto bytes = toBytes(proto);
    if (!bytes.empty()) {
        return std::make_shared<VectorBlob>(std::move(bytes));
    }
    return {};
}

} // namespace LiveKitCpp

namespace {

VectorBlob::VectorBlob(std::vector<uint8_t> data)
    : _data(std::move(data))
{
}

}
