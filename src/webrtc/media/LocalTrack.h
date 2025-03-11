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
#pragma once // LocalTrack.h
#include <api/rtp_sender_interface.h>

namespace LiveKitCpp
{

struct AddTrackRequest;

class LocalTrack
{
public:
    virtual ~LocalTrack() = default;
    // client track ID, equal to WebRTC track ID
    virtual std::string cid() const = 0;
    // server track ID, received from TrackPublishedResponse
    virtual std::string sid() const = 0;
    virtual cricket::MediaType mediaType() const noexcept = 0;
    bool muted() const noexcept { return !enabled(); }
    virtual bool enabled() const noexcept = 0;
    virtual bool fillRequest(AddTrackRequest& request) const = 0;
};

} // namespace LiveKitCpp
