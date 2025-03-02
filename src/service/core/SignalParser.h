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
#pragma once // SignalParser.h
#include "rtc/JoinResponse.h"
#include "livekit_rtc.pb.h"
#include <optional>

namespace LiveKitCpp
{

class SignalParser
{
public:
    template <typename TProtoBufType>
    static std::optional<TProtoBufType> toProto(const void* data, size_t dataLen) {
        if (data && dataLen) {
            TProtoBufType instance;
            if (instance.ParseFromArray(data, int(dataLen))) {
                return instance;
            }
        }
        return std::nullopt;
    }
    static std::optional<livekit::SignalResponse> parse(const void* data, size_t dataLen);
    static JoinResponse fromProto(const livekit::JoinResponse& in);
    static Room fromProto(const livekit::Room& in);
    static Codec fromProto(const livekit::Codec& in);
    static TimedVersion fromProto(const livekit::TimedVersion& in);
    static ParticipantInfo fromProto(const livekit::ParticipantInfo& in);
    static ParticipantKind fromProto(livekit::ParticipantInfo_Kind in);
    static ParticipantState fromProto(livekit::ParticipantInfo_State in);
    static ParticipantPermission fromProto(const livekit::ParticipantPermission& in);
    static DisconnectReason fromProto(livekit::DisconnectReason in);
    static TrackSource fromProto(livekit::TrackSource in);
    static TrackInfo fromProto(const livekit::TrackInfo& in);
    static VideoQuality fromProto(livekit::VideoQuality in);
    static VideoLayer fromProto(const livekit::VideoLayer& in);
    static TrackType fromProto(livekit::TrackType in);
    static SimulcastCodecInfo fromProto(const livekit::SimulcastCodecInfo& in);
    static BackupCodecPolicy fromProto(livekit::BackupCodecPolicy in);
    static EncryptionType fromProto(livekit::Encryption_Type in);
    static AudioTrackFeature fromProto(livekit::AudioTrackFeature in);
private:
    template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
    static std::vector<TCppType> fromProto(const TProtoBufRepeated& in);
};

} // namespace LiveKitCpp
