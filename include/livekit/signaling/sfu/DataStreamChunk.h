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
#pragma once // DataStreamChunk.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

struct DataStreamChunk
{
    // unique identifier for this data stream to map it to the correct header
    std::string _streamId;
    uint64_t _chunkIndex = {};
    // content as binary (bytes)
    std::string _content;
    // a version indicating that this chunk_index has been
    // retroactively modified and the original one needs to be replaced
    int32_t _version = {};
    // optional, initialization vector for AES-GCM encryption
    std::optional<std::string> _iv;
};

} // namespace LiveKitCpp
