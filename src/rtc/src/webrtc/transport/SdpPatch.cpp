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
#include "SdpPatch.h"
#include "Utils.h"
#include <api/jsep.h>
#include <media/base/media_constants.h>
#include <media/base/rtp_utils.h>
#include <media/base/codec.h>
#include <pc/media_session.h>
#include <cassert>

namespace LiveKitCpp
{

SdpPatch::SdpPatch(webrtc::SessionDescriptionInterface* session)
    : _session(session)
{
}

bool SdpPatch::hasSendStreams(cricket::MediaType kind) const
{
    const auto content = contentDescription(kind);
    return content && !content->streams().empty();
}

bool SdpPatch::setRtpTransceiverDirection(webrtc::RtpTransceiverDirection direction, cricket::MediaType kind)
{
    const auto content = contentDescription(kind);
    if (content && direction != content->direction()) {
        content->set_direction(direction);
        return true;
    }
    return false;
}

bool SdpPatch::setCodec(std::string_view codecName, cricket::MediaType kind)
{
    bool changed = false;
    if (!codecName.empty()) {
        if (const auto content = contentDescription(kind)) {
            switch (kind) {
                case cricket::MediaType::MEDIA_TYPE_VIDEO:
                    changed = moveCodecProfilesToFront(content->as_video(), std::move(codecName));
                    break;
                case cricket::MediaType::MEDIA_TYPE_AUDIO:
                    changed = moveCodecProfilesToFront(content->as_audio(), std::move(codecName));
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }
    return changed;
}

bool SdpPatch::setMediaBandwidth(int bps, cricket::MediaType kind)
{
    if (const auto content = contentDescription(kind)) {
        if (bps > 0) {
            content->set_bandwidth(bps);
        } else {
            content->set_bandwidth(cricket::kAutoBandwidth);
        }
        return true;
    }
    return false;
}

template <class TContentDescription>
bool SdpPatch::moveCodecProfilesToFront(TContentDescription* desc, std::string_view codecName)
{
    bool changed = false;
    if (desc && !codecName.empty()) {
        auto availableCodecs = desc->codecs();
        if (!availableCodecs.empty()) {
            AssociatedPayloads payloads;
            for (const auto& availableCodec : availableCodecs) {
                if (compareCaseInsensitive(availableCodec.name, codecName)) {
                    payloads.insert(availableCodec.id);
                }
            }
            if (!payloads.empty()) {
                for (auto it = availableCodecs.begin(), swapIt = it; it != availableCodecs.end(); ++it) {
                    if (isAssociatedCodec(payloads, *it) && swapIt != it) {
                        std::swap(*it, *swapIt++);
                        changed = true;
                    }
                }
            }
            if (changed) {
                desc->set_codecs(availableCodecs);
            }
        }
    }
    return changed;
}

cricket::MediaContentDescription* SdpPatch::contentDescription(cricket::MediaType kind) const
{
    if (_session && _session->description()) {
        for (auto& content : _session->description()->contents()) {
            if (content.media_description() && content.media_description()->type() == kind) {
                return content.media_description();
            }
        }
    }
    return nullptr;
}

bool SdpPatch::isAssociatedCodec(const AssociatedPayloads& payloads, const cricket::Codec& codec)
{
    if (cricket::Codec::ResiliencyType::kNone != codec.GetResiliencyType()) {
        int associatedPayloadType = 0;
        if (codec.GetParam(cricket::kCodecParamAssociatedPayloadType, &associatedPayloadType) &&
            cricket::IsValidRtpPayloadType(associatedPayloadType)) {
                return payloads.count(associatedPayloadType) > 0UL;
        }
        return false;
    }
    return payloads.count(codec.id) > 0UL;
}

} // namespace LiveKitCpp
