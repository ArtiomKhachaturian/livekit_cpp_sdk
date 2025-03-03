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
#include "rtc/ConnectionQualityUpdate.h"
#include "rtc/JoinResponse.h"
#include "rtc/LeaveRequest.h"
#include "rtc/MuteTrackRequest.h"
#include "rtc/TrickleRequest.h"
#include "rtc/ParticipantUpdate.h"
#include "rtc/RoomUpdate.h"
#include "rtc/SpeakersChanged.h"
#include "rtc/StreamStateUpdate.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"
#include "rtc/SubscribedQualityUpdate.h"
#include "rtc/ReconnectResponse.h"
#include "rtc/TrackSubscribed.h"
#include "rtc/RequestResponse.h"
#include "rtc/SubscriptionResponse.h"
#include "livekit_rtc.pb.h"
#include <optional>
#include <unordered_map>

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
    // responses & requests
    static std::optional<livekit::SignalResponse> parse(const void* data, size_t dataLen);
    static JoinResponse from(const livekit::JoinResponse& in);
    static TrickleRequest from(const livekit::TrickleRequest& in);
    static ParticipantUpdate from(const livekit::ParticipantUpdate& in);
    static TrackPublishedResponse from(const livekit::TrackPublishedResponse& in);
    static TrackUnpublishedResponse from(const livekit::TrackUnpublishedResponse& in);
    static LeaveRequest from(const livekit::LeaveRequest& in);
    static MuteTrackRequest from(const livekit::MuteTrackRequest& in);
    static SpeakersChanged from(const livekit::SpeakersChanged& in);
    static RoomUpdate from(const livekit::RoomUpdate& in);
    static ConnectionQualityUpdate from(const livekit::ConnectionQualityUpdate& in);
    static StreamStateUpdate from(const livekit::StreamStateUpdate& in);
    static SubscribedQualityUpdate from(const livekit::SubscribedQualityUpdate& in);
    static ReconnectResponse from(const livekit::ReconnectResponse& in);
    static TrackSubscribed from(const livekit::TrackSubscribed& in);
    static RequestResponse from(const livekit::RequestResponse& in);
    static SubscriptionResponse from(const livekit::SubscriptionResponse& in);
    // data
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
    static LeaveRequestAction from(livekit::LeaveRequest_Action in);
    static RegionInfo from(const livekit::RegionInfo& in);
    static RegionSettings from(const livekit::RegionSettings& in);
    static SpeakerInfo from(const livekit::SpeakerInfo& in);
    static ConnectionQuality from(livekit::ConnectionQuality in);
    static ConnectionQualityInfo from(const livekit::ConnectionQualityInfo& in);
    static StreamState from(livekit::StreamState in);
    static StreamStateInfo from(const livekit::StreamStateInfo& in);
    static SubscribedQuality from(const livekit::SubscribedQuality& in);
    static SubscribedCodec from(const livekit::SubscribedCodec& in);
    static ICEServer from(const livekit::ICEServer& in);
    static RequestResponseReason from(livekit::RequestResponse_Reason in);
    static SubscriptionError from(livekit::SubscriptionError in);
private:
    template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
    static std::vector<TCppType> from(const TProtoBufRepeated& in);
    template<typename K, typename V>
    static std::unordered_map<K, V> from(const google::protobuf::Map<K, V>& in);
};

} // namespace LiveKitCpp
