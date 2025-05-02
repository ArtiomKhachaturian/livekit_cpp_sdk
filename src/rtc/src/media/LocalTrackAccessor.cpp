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
#include "LocalTrackAccessor.h"

namespace LiveKitCpp
{

std::string LocalTrackAccessor::cid() const
{
    if (const auto& m = media()) {
        return m->id();
    }
    return {};
}

webrtc::MediaType LocalTrackAccessor::mediaType() const
{
    const auto kind = this->kind();
    if (webrtc::MediaTypeToString(webrtc::MediaType::AUDIO) == kind) {
        return webrtc::MediaType::AUDIO;
    }
    if (webrtc::MediaTypeToString(webrtc::MediaType::VIDEO) == kind) {
        return webrtc::MediaType::VIDEO;
    }
    return webrtc::MediaType::UNSUPPORTED;
}

std::string LocalTrackAccessor::kind() const
{
    if (const auto& m = media()) {
        return m->kind();
    }
    return {};
}

} // namespace LiveKitCpp
