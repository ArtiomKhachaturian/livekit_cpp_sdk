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
#pragma once // DataStreamHeader.h
#include "livekit/signaling/sfu/DataStreamByteHeader.h"
#include "livekit/signaling/sfu/DataStreamTextHeader.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include <unordered_map>
#include <optional>
#include <variant>

namespace LiveKitCpp
{

// main DataStream.Header that contains a oneof for specific headers
struct DataStreamHeader
{
    // unique identifier for this data stream
    std::string _streamId;
    // using int64 for Unix timestamp
    int64_t _timestamp = {};
    std::string _topic;
    std::string _mimeType;
    // only populated for finite streams,
    // if it's a stream of unknown size this stays empty
    std::optional<uint64_t> _totalLength;
    // defaults to NONE
    EncryptionType _encryptionType = EncryptionType::None;
    // user defined attributes map that can carry additional info
    std::unordered_map<std::string, std::string> _attributes;
    // oneof to choose between specific header types
    std::variant<std::nullptr_t,
                 DataStreamByteHeader,
                 DataStreamTextHeader> _contentHeader;
};

} // namespace LiveKitCpp
