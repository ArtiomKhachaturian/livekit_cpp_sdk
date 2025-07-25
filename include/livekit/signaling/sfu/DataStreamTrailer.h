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
#pragma once // DataStreamTrailer.h
#include <string>
#include <unordered_map>

namespace LiveKitCpp
{

struct DataStreamTrailer
{
    // unique identifier for this data stream
    std::string _streamId;
    // reason why the stream was closed
    // (could contain "error" / "interrupted" / empty for expected end)
    std::string _reason;
    // finalizing updates for the stream, can also include additional
    // insights for errors or endTime for transcription
    std::unordered_map<std::string, std::string> _attributes;
};

} // namespace LiveKitCpp
