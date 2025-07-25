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
#pragma once // StatsDataChannelExt.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

// https://www.w3.org/TR/webrtc-stats/#dom-rtcdatachannelstats
class StatsDataChannelExt
{
public:
    virtual ~StatsDataChannelExt() = default;
    virtual std::optional<std::string> label() const = 0;
    virtual std::optional<std::string> protocol() const = 0;
    virtual std::optional<int32_t> dataChannelIdentifier() const = 0;
    virtual std::optional<std::string> state() const = 0;
    virtual std::optional<uint32_t> messagesSent() const = 0;
    virtual std::optional<uint64_t> bytesSent() const = 0;
    virtual std::optional<uint32_t> messagesReceived() const = 0;
    virtual std::optional<uint64_t> bytesReceived() const = 0;
};

} // namespace LiveKitCpp
