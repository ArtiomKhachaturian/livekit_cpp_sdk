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
#include "State.h"
#include "WebsocketCloseCode.h"
#include "WebsocketOptions.h"
#include "CommandSender.h"
#include <string>

namespace LiveKitCpp
{

class WebsocketListener;

class LIVEKIT_CLIENT_API Websocket : public CommandSender
{
public:
    ~Websocket() override = default;
    virtual void addListener(WebsocketListener* listener) = 0;
    virtual void removeListener(WebsocketListener* listener) = 0;
    // [connectionId] will passed to all methods of WebsocketListener
    virtual bool open(WebsocketOptions options, uint64_t connectionId = 0U) = 0;
    virtual void close() = 0;
    virtual std::string host() const = 0;
    virtual State state() const = 0;
    uint64_t id() const noexcept { return reinterpret_cast<uint64_t>(this); }
};


} // namespace LiveKitCpp
