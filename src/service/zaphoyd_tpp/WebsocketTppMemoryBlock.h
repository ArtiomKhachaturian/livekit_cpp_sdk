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
#pragma once // WebsocketTppMemoryBlock.h
#include "MemoryBlock.h"

namespace LiveKitCpp
{

template<class MessagePtr>
class WebsocketTppMemoryBlock : public MemoryBlock
{
public:
    WebsocketTppMemoryBlock(MessagePtr message);
    ~WebsocketTppMemoryBlock() final;
private:
    const MessagePtr _message;
};

template<class MessagePtr>
WebsocketTppMemoryBlock<MessagePtr>::WebsocketTppMemoryBlock(MessagePtr message)
    : _message(std::move(message))
{
    auto& payload = _message->get_raw_payload();
    setData(reinterpret_cast<uint8_t*>(payload.data()), payload.size());
}

template<class MessagePtr>
WebsocketTppMemoryBlock<MessagePtr>::~WebsocketTppMemoryBlock()
{
    _message->recycle();
}

} // namespace LiveKitCpp
