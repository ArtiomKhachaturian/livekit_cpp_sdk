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
#include "livekit/signaling/SignalClient.h"
#include "ResponseInterceptor.h"
#include "RequestSender.h"
#include "Blob.h"

namespace LiveKitCpp
{

SignalClient::SignalClient(CommandSender* commandSender, Bricks::Logger* logger)
    :  Bricks::LoggableR<>(logger)
    , _responseReceiver(std::make_unique<ResponseInterceptor>(logger))
    , _requestSender(std::make_unique<RequestSender>(commandSender, logger))
{
}

SignalClient::~SignalClient()
{
}

void SignalClient::setServerListener(SignalServerListener* listener)
{
    _responseReceiver->setListener(listener);
}

bool SignalClient::sendOffer(const SessionDescription& sdp) const
{
    return _requestSender->offer(sdp);
}

bool SignalClient::sendAnswer(const SessionDescription& sdp) const
{
    return _requestSender->answer(sdp);
}

bool SignalClient::sendTrickle(const TrickleRequest& request) const
{
    return _requestSender->trickle(request);
}

bool SignalClient::sendAddTrack(const AddTrackRequest& request) const
{
    return _requestSender->addTrack(request);
}

bool SignalClient::sendMuteTrack(const MuteTrackRequest& request) const
{
    return _requestSender->muteTrack(request);
}

bool SignalClient::sendSubscription(const UpdateSubscription& update) const
{
    return _requestSender->subscription(update);
}

bool SignalClient::sendTrackSettings(const UpdateTrackSettings& update) const
{
    return _requestSender->trackSettings(update);
}

bool SignalClient::sendLeave(const LeaveRequest& request) const
{
    return _requestSender->leave(request);
}

bool SignalClient::sendUpdateVideoLayers(const UpdateVideoLayers& update) const
{
    return _requestSender->updateVideoLayers(update);
}

bool SignalClient::sendSubscriptionPermission(const SubscriptionPermission& permission) const
{
    return _requestSender->subscriptionPermission(permission);
}

bool SignalClient::sendSyncState(const SyncState& state) const
{
    return _requestSender->syncState(state);
}

bool SignalClient::sendSimulate(const SimulateScenario& scenario) const
{
    return _requestSender->simulate(scenario);
}

bool SignalClient::sendUpdateMetadata(const UpdateParticipantMetadata& data) const
{
    return _requestSender->updateMetadata(data);
}

bool SignalClient::sendPingReq(const Ping& ping) const
{
    return _requestSender->pingReq(ping);
}

bool SignalClient::sendUpdateAudioTrack(const UpdateLocalAudioTrack& track) const
{
    return _requestSender->updateAudioTrack(track);
}

bool SignalClient::sendUpdateVideoTrack(const UpdateLocalVideoTrack& track) const
{
    return _requestSender->updateVideoTrack(track);
}

void SignalClient::handleServerProtobufMessage(const Bricks::Blob& message)
{
    if (message) {
        _responseReceiver->parseBinary(message.data(), message.size());
    }
}

std::string_view SignalClient::logCategory() const
{
    static const std::string_view category("signaling_client");
    return category;
}

} // namespace LiveKitCpp
