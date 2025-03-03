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
#pragma once // RequestSender.h
#include <memory>
#include <vector>

namespace google::protobuf {
class MessageLite;
}

namespace LiveKitCpp
{

class CommandSender;
class MemoryBlock;
struct SessionDescription;

class RequestSender
{
public:
    RequestSender(CommandSender* commandSender);
    bool sendOffer(const SessionDescription& offer) const;
    bool sendAnswer(const SessionDescription& answer) const;
private:
    bool canSend() const;
    bool sendSdp(const SessionDescription& sdp, bool offer) const;
    template <class TSetMethod, class TObject>
    bool mapAndSendRequest(const TSetMethod& setMethod, const TObject& object) const;
    static std::vector<uint8_t> toBytes(const google::protobuf::MessageLite& proto);
    static std::shared_ptr<MemoryBlock> toMemBlock(const google::protobuf::MessageLite& proto);
private:
    CommandSender* const _commandSender;
};

} // namespace LiveKitCpp
