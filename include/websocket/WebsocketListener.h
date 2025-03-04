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
#pragma once
#include "LiveKitClientExport.h"
#include <string_view>
#include <memory>

namespace LiveKitCpp
{

class MemoryBlock;
class WebsocketError;
enum class State;

class LIVEKIT_CLIENT_API WebsocketListener
{
public:
    virtual void onStateChanged(uint64_t /*socketId*/, uint64_t /*connectionId*/,
                                const std::string_view& /*host*/,
                                State /*state*/) {}
    virtual void onError(uint64_t /*socketId*/, uint64_t /*connectionId*/,
                         const std::string_view& /*host*/,
                         const WebsocketError& /*error*/) {}
    virtual void onTextMessageReceived(uint64_t /*socketId*/, uint64_t /*connectionId*/,
                                       const std::string_view& /*message*/) {}
    virtual void onBinaryMessageReceved(uint64_t /*socketId*/, uint64_t /*connectionId*/,
                                        const std::shared_ptr<const MemoryBlock>& /*message*/) {}
protected:
    virtual ~WebsocketListener() = default;
};

} // namespace LiveKitCpp
