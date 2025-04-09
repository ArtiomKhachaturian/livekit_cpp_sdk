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
#pragma once // RemoteVideoTrack.h
#include "media/VideoTrack.h"
#include "rtc/VideoLayer.h"
#include "rtc/SimulcastCodecInfo.h"
#include <vector>

namespace LiveKitCpp
{

class RemoteVideoTrack : public VideoTrack
{
public:
    // server track ID
    virtual std::string sid() const = 0;
    // clients may receive a lower resolution version with simulcast
    virtual uint32_t originalWidth() const = 0;
    virtual uint32_t originalHeight() const = 0;
    virtual std::vector<VideoLayer> layers() const = 0;
    virtual std::vector<SimulcastCodecInfo> codecs() const = 0;
    virtual std::string mime() const = 0;
    virtual std::string stream() const = 0;
};

} // namespace LiveKitCpp
