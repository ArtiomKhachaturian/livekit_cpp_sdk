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

enum class SetSenderResult
{
    Accepted,
    Rejected,
    NotMatchedToRequest
};

class LocalTrack
{
public:
    virtual ~LocalTrack() = default;
    virtual cricket::MediaType mediaType() const noexcept = 0;
    virtual bool enabled() const noexcept = 0;
    virtual void setEnabled(bool enable) = 0;
    virtual SetSenderResult setRequested(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) = 0;
    virtual bool fillRequest(AddTrackRequest& request) const = 0;
};

} // namespace LiveKitCpp
