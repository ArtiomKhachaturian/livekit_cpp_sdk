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
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include "CommandSender.h"

namespace LiveKitCpp
{

enum class State;

class WebsocketTppApi : public CommandSender
{
public:
    ~WebsocketTppApi() override = default;
    virtual bool open() = 0;
    virtual std::string host() const = 0;
    virtual State state() const = 0;
    virtual void destroy() = 0;
};

} // namespace LiveKitCpp
#endif
