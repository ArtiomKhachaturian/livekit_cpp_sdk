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
#pragma once
#include <api/media_types.h>
#include <memory>
#include <string>
#include <optional>

namespace webrtc {
enum class RtpTransceiverDirection;
struct SdpParseError;
class SessionDescriptionInterface;
} // namespace webrtc

namespace cricket {
class MediaContentDescription;
struct Codec;
} // namespace cricket

namespace LiveKitCpp
{

class SdpPatch
{
public:
    SdpPatch(webrtc::SessionDescriptionInterface* session);
    bool hasSendStreams(cricket::MediaType kind) const;
    // return true if changed
    bool setRtpTransceiverDirection(webrtc::RtpTransceiverDirection direction, cricket::MediaType kind);
    bool setCodec(std::string_view codecName, cricket::MediaType kind);
    //in bits
    bool setMediaBandwidth(int bps, cricket::MediaType kind);
    const webrtc::SessionDescriptionInterface* session() const { return _session; }
private:
    cricket::MediaContentDescription* contentDescription(cricket::MediaType kind) const;
    static bool moveCodecProfilesToFront(cricket::MediaContentDescription* desc, std::string_view codecName);
    static bool isAssociatedCodec(int payload, const cricket::Codec& codec);
private:
    webrtc::SessionDescriptionInterface* const _session;
};

} // namespace LiveKitCpp
