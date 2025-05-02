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
#include <set>

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
                    changed = moveCodecProfilesToFront(content, std::move(codecName));
                    break;
                case cricket::MediaType::MEDIA_TYPE_AUDIO:
                    changed = moveCodecProfilesToFront(content, std::move(codecName));
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

bool SdpPatch::moveCodecProfilesToFront(webrtc::MediaContentDescription* desc, std::string_view codecName)
{
    bool changed = false;
    if (desc && !codecName.empty() && !desc->codecs().empty()) {
        std::set<int> payloads;
        for (const auto& codec : desc->codecs()) {
            if (compareCaseInsensitive(codec.name, codecName)) {
                payloads.insert(codec.id);
            }
        }
        if (!payloads.empty()) {
            std::vector<cricket::Codec> input(desc->codecs()), output;
            output.reserve(input.size());
            for (int payload : payloads) {
                auto it = std::find_if(input.begin(), input.end(), [payload](const auto& codec) {
                    return codec.id == payload;
                });
                if (it != input.end()) {
                    output.push_back(std::move(*it));
                    input.erase(it);
                    for (it = input.begin(); it != input.end();) {
                        if (isAssociatedCodec(payload, *it)) {
                            output.push_back(std::move(*it));
                            input.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                }
            }
            // copy rest of codecs
            output.insert(output.end(), std::make_move_iterator(input.begin()), std::make_move_iterator(input.end()));
            changed = output != desc->codecs();
            if (changed) {
                desc->set_codecs(output);
            }
        }
    }
    return changed;
}

webrtc::MediaContentDescription* SdpPatch::contentDescription(cricket::MediaType kind) const
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

bool SdpPatch::isAssociatedCodec(int payload, const cricket::Codec& codec)
{
    if (cricket::Codec::ResiliencyType::kNone != codec.GetResiliencyType()) {
        int associatedPayloadType = 0;
        if (codec.GetParam(cricket::kCodecParamAssociatedPayloadType, &associatedPayloadType) &&
            cricket::IsValidRtpPayloadType(associatedPayloadType)) {
                return payload == associatedPayloadType;
        }
    }
    return false;
}

} // namespace LiveKitCpp
