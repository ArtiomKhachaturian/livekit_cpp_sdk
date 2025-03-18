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
#include "ProtoUtils.h"

namespace LiveKitCpp
{

std::vector<uint8_t> protoToBytes(const google::protobuf::Message& proto,
                                  Bricks::Logger* logger,
                                  std::string_view category)
{
    std::vector<uint8_t> buffer;
    if (const auto size = proto.ByteSizeLong()) {
        buffer.resize(size);
        if (!proto.SerializeToArray(buffer.data(), int(size))) {
            if (logger && logger->canLogError()) {
                logger->logError(std::string("failed serialize of ") +
                                 proto.GetTypeName() + " to blob", category);
            }
            buffer.clear();
        }
    }
    return buffer;
}

} // namespace LiveKitCpp
