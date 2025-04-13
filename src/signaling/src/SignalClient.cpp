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
#include "ResponseReceiver.h"
#include "RequestSender.h"
#include "Blob.h"

namespace LiveKitCpp
{

SignalClient::SignalClient(CommandSender* commandSender, Bricks::Logger* logger)
    :  Bricks::LoggableR<>(logger)
    , _responseReceiver(std::make_unique<ResponseReceiver>(logger))
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

void SignalClient::parseProtobuBlob(const Bricks::Blob& message)
{
    parseProtobufData(message.data(), message.size());
}

void SignalClient::parseProtobufData(const void* data, size_t dataLen)
{
    _responseReceiver->parseBinary(data, dataLen);
}

bool SignalClient::sendOffer(SessionDescription sdp) const
{
    return _requestSender->offer(std::move(sdp));
}

bool SignalClient::sendAnswer(SessionDescription sdp) const
{
    return _requestSender->answer(std::move(sdp));
}

bool SignalClient::sendTrickle(TrickleRequest request) const
{
    return _requestSender->trickle(std::move(request));
}

bool SignalClient::sendAddTrack(AddTrackRequest request) const
{
    return _requestSender->addTrack(std::move(request));
}

bool SignalClient::sendMuteTrack(MuteTrackRequest request) const
{
    return _requestSender->muteTrack(std::move(request));
}

bool SignalClient::sendSubscription(UpdateSubscription update) const
{
    return _requestSender->subscription(std::move(update));
}

bool SignalClient::sendTrackSettings(UpdateTrackSettings update) const
{
    return _requestSender->trackSettings(std::move(update));
}

bool SignalClient::sendLeave(LeaveRequest request) const
{
    return _requestSender->leave(std::move(request));
}

bool SignalClient::sendUpdateVideoLayers(UpdateVideoLayers update) const
{
    return _requestSender->updateVideoLayers(std::move(update));
}

bool SignalClient::sendSubscriptionPermission(SubscriptionPermission permission) const
{
    return _requestSender->subscriptionPermission(std::move(permission));
}

bool SignalClient::sendSyncState(SyncState state) const
{
    return _requestSender->syncState(std::move(state));
}

bool SignalClient::sendSimulate(SimulateScenario scenario) const
{
    return _requestSender->simulate(std::move(scenario));
}

bool SignalClient::sendUpdateMetadata(UpdateParticipantMetadata data) const
{
    return _requestSender->updateMetadata(std::move(data));
}

bool SignalClient::sendPingReq(Ping ping) const
{
    return _requestSender->pingReq(std::move(ping));
}

bool SignalClient::sendUpdateAudioTrack(UpdateLocalAudioTrack track) const
{
    return _requestSender->updateAudioTrack(std::move(track));
}

bool SignalClient::sendUpdateVideoTrack(UpdateLocalVideoTrack track) const
{
    return _requestSender->updateVideoTrack(std::move(track));
}

bool SignalClient::sendDataPacket(DataPacket packet) const
{
    return _requestSender->dataPacket(std::move(packet));
}

void SignalClient::notifyAboutError(std::string details)
{
    _responseReceiver->notifyAboutError(std::move(details));
}

std::string_view SignalClient::logCategory() const
{
    static const std::string_view category("signaling_client");
    return category;
}

} // namespace LiveKitCpp
