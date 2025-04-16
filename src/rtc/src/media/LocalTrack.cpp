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
#include "LocalTrack.h"

namespace LiveKitCpp
{

std::string LocalTrack::cid() const
{
    if (const auto& m = media()) {
        return m->id();
    }
    return {};
}

cricket::MediaType LocalTrack::mediaType() const
{
    const auto kind = this->kind();
    if (cricket::MediaTypeToString(cricket::MEDIA_TYPE_AUDIO) == kind) {
        return cricket::MEDIA_TYPE_AUDIO;
    }
    if (cricket::MediaTypeToString(cricket::MEDIA_TYPE_VIDEO) == kind) {
        return cricket::MEDIA_TYPE_VIDEO;
    }
    return cricket::MEDIA_TYPE_UNSUPPORTED;
}

std::string LocalTrack::kind() const
{
    if (const auto& m = media()) {
        return m->kind();
    }
    return {};
}

} // namespace LiveKitCpp
