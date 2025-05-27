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
#include <media/base/codec.h>
#include <memory>
#include <string>
#include <optional>

namespace webrtc {
enum class RtpTransceiverDirection;
struct SdpParseError;
class SessionDescriptionInterface;
class MediaContentDescription;
} // namespace webrtc

namespace cricket {
struct Codec;
} // namespace cricket

namespace LiveKitCpp
{

class SdpPatch
{
public:
    SdpPatch(webrtc::SessionDescriptionInterface* session);
    void setCodec(std::string_view codecName, webrtc::MediaType kind);
    //in bits
    void setMediaBandwidth(int bps, webrtc::MediaType kind);
private:
    std::vector<webrtc::MediaContentDescription*> descriptions(webrtc::MediaType kind) const;
    static void moveCodecProfilesToFront(webrtc::MediaContentDescription* desc, std::string_view codecName);
    static bool isAssociatedCodec(int payload, const webrtc::Codec& codec);
private:
    webrtc::SessionDescriptionInterface* const _session;
};

} // namespace LiveKitCpp
