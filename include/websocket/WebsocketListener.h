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
#include <string_view>
#include <memory>

namespace Websocket
{

class Blob;
class Error;
enum class State;

class Listener
{
public:
    virtual void onStateChanged(uint64_t /*socketId*/,
                                uint64_t /*connectionId*/,
                                State /*state*/) {}
    virtual void onError(uint64_t /*socketId*/,
                         uint64_t /*connectionId*/,
                         const Error& /*error*/) {}
    virtual void onTextMessage(uint64_t /*socketId*/,
                               uint64_t /*connectionId*/,
                               const std::string_view& /*message*/) {}
    virtual void onBinaryMessage(uint64_t /*socketId*/,
                                 uint64_t /*connectionId*/,
                                 const std::shared_ptr<Blob>& /*message*/) {}
protected:
    virtual ~Listener() = default;
};

} // namespace Websocket
