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
#include "MemoryBlock.h"
#include "Signals.h"
#include "google/protobuf/message_lite.h"

namespace LiveKitCpp
{

RequestSender::RequestSender(CommandSender* commandSender)
    : _commandSender(commandSender)
{
}

bool RequestSender::sendOffer(const SessionDescription& offer) const
{
    return sendSdp(offer, true);
}

bool RequestSender::sendAnswer(const SessionDescription& answer) const
{
    return sendSdp(answer, false);
}

bool RequestSender::canSend() const
{
    return nullptr != _commandSender;
}

bool RequestSender::sendSdp(const SessionDescription& sdp, bool offer) const
{
    if (offer) {
        return mapAndSendRequest(&livekit::SignalRequest::set_allocated_offer, sdp);
    }
    return mapAndSendRequest(&livekit::SignalRequest::set_allocated_answer, sdp);
}

template <class TSetMethod, class TObject>
bool RequestSender::mapAndSendRequest(const TSetMethod& setMethod, const TObject& object) const
{
    if (canSend()) {
        livekit::SignalRequest request;
        (request.*setMethod)(Signals::map(object));
        if (const auto block = toMemBlock(request)) {
            return _commandSender->sendBinary(block);
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

std::shared_ptr<MemoryBlock> RequestSender::toMemBlock(const google::protobuf::MessageLite& proto)
{
    return MemoryBlock::make(toBytes(proto));
}

} // namespace LiveKitCpp
