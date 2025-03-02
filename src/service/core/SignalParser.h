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
#include "rtc/TrickleRequest.h"
#include "rtc/ParticipantUpdate.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"
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
    static JoinResponse from(const livekit::JoinResponse& in);
    static TrickleRequest from(const livekit::TrickleRequest& in);
    static ParticipantUpdate from(const livekit::ParticipantUpdate& in);
    static TrackPublishedResponse from(const livekit::TrackPublishedResponse& in);
    static TrackUnpublishedResponse from(const livekit::TrackUnpublishedResponse& in);
    static Room from(const livekit::Room& in);
    static Codec from(const livekit::Codec& in);
    static TimedVersion from(const livekit::TimedVersion& in);
    static ParticipantInfo from(const livekit::ParticipantInfo& in);
    static ParticipantKind from(livekit::ParticipantInfo_Kind in);
    static ParticipantState from(livekit::ParticipantInfo_State in);
    static ParticipantPermission from(const livekit::ParticipantPermission& in);
    static DisconnectReason from(livekit::DisconnectReason in);
    static TrackSource from(livekit::TrackSource in);
    static TrackInfo from(const livekit::TrackInfo& in);
    static VideoQuality from(livekit::VideoQuality in);
    static VideoLayer from(const livekit::VideoLayer& in);
    static TrackType from(livekit::TrackType in);
    static SimulcastCodecInfo from(const livekit::SimulcastCodecInfo& in);
    static BackupCodecPolicy from(livekit::BackupCodecPolicy in);
    static EncryptionType from(livekit::Encryption_Type in);
    static AudioTrackFeature from(livekit::AudioTrackFeature in);
    static ClientConfigSetting from(livekit::ClientConfigSetting in);
    static ClientConfiguration from(const livekit::ClientConfiguration& in);
    static DisabledCodecs from(const livekit::DisabledCodecs& in);
    static VideoConfiguration from(const livekit::VideoConfiguration& in);
    static ServerEdition from(livekit::ServerInfo_Edition in);
    static ServerInfo from(const livekit::ServerInfo& in);
    static SignalTarget from(livekit::SignalTarget in);
private:
    template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
    static std::vector<TCppType> from(const TProtoBufRepeated& in);
};

} // namespace LiveKitCpp
