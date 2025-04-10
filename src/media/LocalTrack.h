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
#ifdef WEBRTC_AVAILABLE
#include <api/media_stream_interface.h>
#include <api/media_types.h>
#include <string>

namespace webrtc {
class RtpSenderInterface;
}

namespace LiveKitCpp
{

struct AddTrackRequest;

class LocalTrack
{
public:
    std::string cid() const;
    cricket::MediaType mediaType() const;
    virtual void setRemoteSideMute(bool mute) = 0;
    // Terminates all media, closes the capturers, and in general releases any
    // resources used by the local track. This is an irreversible operation.
    virtual void close() {}
    // for publishing
    virtual void setSid(const std::string& sid) = 0;
    virtual bool fillRequest(AddTrackRequest* request) const = 0;
    // transport layer
    virtual webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> media() const = 0;
    virtual void notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender,
                                                 bool encryption) = 0;
    virtual void notifyThatMediaRemovedFromTransport() = 0;
    // state
    virtual bool muted() const = 0;
protected:
    ~LocalTrack() = default;
};

} // namespace LiveKitCpp
#endif
