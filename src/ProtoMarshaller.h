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
#pragma once // Signals.h
#include "Loggable.h"
#include "rtc/ConnectionQualityUpdate.h"
#include "rtc/JoinResponse.h"
#include "rtc/SessionDescription.h"
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
#include "rtc/SubscriptionPermissionUpdate.h"
#include "rtc/AddTrackRequest.h"
#include "rtc/UpdateSubscription.h"
#include "rtc/UpdateTrackSettings.h"
#include "rtc/UpdateVideoLayers.h"
#include "rtc/SubscriptionPermission.h"
#include "rtc/SyncState.h"
#include "rtc/SimulateScenario.h"
#include "rtc/UpdateParticipantMetadata.h"
#include "rtc/Ping.h"
#include "rtc/Pong.h"
#include "rtc/UpdateLocalAudioTrack.h"
#include "rtc/UpdateLocalVideoTrack.h"
#include "rtc/ClientInfo.h"
#include "livekit_rtc.pb.h"
#include "livekit_models.pb.h"
#include <optional>
#include <unordered_map>

namespace LiveKitCpp
{

class ProtoMarshaller : public Bricks::LoggableR<>
{
public:
    ProtoMarshaller(Bricks::Logger* logger = nullptr);
    std::vector<uint8_t> toBytes(const google::protobuf::Message& proto) const;
    template <typename TProto>
    std::optional<TProto> fromBytes(const void* data, size_t dataLen) const;
    // responses & requests
    JoinResponse map(const livekit::JoinResponse& in) const;
    SessionDescription map(const livekit::SessionDescription& in) const;
    livekit::SessionDescription map(const SessionDescription& in) const;
    TrickleRequest map(const livekit::TrickleRequest& in) const;
    livekit::TrickleRequest map(const TrickleRequest& in) const;
    ParticipantUpdate map(const livekit::ParticipantUpdate& in) const;
    TrackPublishedResponse map(const livekit::TrackPublishedResponse& in) const;
    livekit::TrackPublishedResponse map(const TrackPublishedResponse& in) const;
    TrackUnpublishedResponse map(const livekit::TrackUnpublishedResponse& in) const;
    LeaveRequest map(const livekit::LeaveRequest& in) const;
    livekit::LeaveRequest map(const LeaveRequest& in) const;
    MuteTrackRequest map(const livekit::MuteTrackRequest& in) const;
    livekit::MuteTrackRequest map(const MuteTrackRequest& in) const;
    SpeakersChanged map(const livekit::SpeakersChanged& in) const;
    RoomUpdate map(const livekit::RoomUpdate& in) const;
    ConnectionQualityUpdate map(const livekit::ConnectionQualityUpdate& in) const;
    StreamStateUpdate map(const livekit::StreamStateUpdate& in) const;
    SubscribedQualityUpdate map(const livekit::SubscribedQualityUpdate& in) const;
    ReconnectResponse map(const livekit::ReconnectResponse& in) const;
    TrackSubscribed map(const livekit::TrackSubscribed& in) const;
    RequestResponse map(const livekit::RequestResponse& in) const;
    SubscriptionResponse map(const livekit::SubscriptionResponse& in) const;
    SubscriptionPermissionUpdate map(const livekit::SubscriptionPermissionUpdate& in) const;
    AddTrackRequest map(const livekit::AddTrackRequest& in) const;
    livekit::AddTrackRequest map(const AddTrackRequest& in) const;
    UpdateSubscription map(const livekit::UpdateSubscription& in) const;
    livekit::UpdateSubscription map(const UpdateSubscription& in) const;
    UpdateTrackSettings map(const livekit::UpdateTrackSettings& in) const;
    livekit::UpdateTrackSettings map(const UpdateTrackSettings& in) const;
    UpdateVideoLayers map(const livekit::UpdateVideoLayers& in) const;
    livekit::UpdateVideoLayers map(const UpdateVideoLayers& in) const;
    SubscriptionPermission map(const livekit::SubscriptionPermission& in) const;
    livekit::SubscriptionPermission map(const SubscriptionPermission& in) const;
    SyncState map(const livekit::SyncState& in) const;
    livekit::SyncState map(const SyncState& in) const;
    SimulateScenario map(const livekit::SimulateScenario& in) const;
    livekit::SimulateScenario map(const SimulateScenario& in) const;
    UpdateParticipantMetadata map(const livekit::UpdateParticipantMetadata& in) const;
    livekit::UpdateParticipantMetadata map(const UpdateParticipantMetadata& in) const;
    Ping map(const livekit::Ping& in) const;
    livekit::Ping map(const Ping& in) const;
    Pong map(const livekit::Pong& in) const;
    livekit::Pong map(const Pong& in) const;
    UpdateLocalAudioTrack map(const livekit::UpdateLocalAudioTrack& in) const;
    livekit::UpdateLocalAudioTrack map(const UpdateLocalAudioTrack& in) const;
    UpdateLocalVideoTrack map(const livekit::UpdateLocalVideoTrack& in) const;
    livekit::UpdateLocalVideoTrack map(const UpdateLocalVideoTrack& in) const;
    ClientInfo map(const livekit::ClientInfo& in) const;
    livekit::ClientInfo map(const ClientInfo& in) const;
    // data
    Room map(const livekit::Room& in) const;
    Codec map(const livekit::Codec& in) const;
    TimedVersion map(const livekit::TimedVersion& in) const;
    livekit::TimedVersion map(const TimedVersion& in) const;
    ParticipantInfo map(const livekit::ParticipantInfo& in) const;
    ParticipantKind map(livekit::ParticipantInfo_Kind in) const;
    ParticipantState map(livekit::ParticipantInfo_State in) const;
    ParticipantPermission map(const livekit::ParticipantPermission& in) const;
    DisconnectReason map(livekit::DisconnectReason in) const;
    livekit::DisconnectReason map(DisconnectReason in) const;
    TrackSource map(livekit::TrackSource in) const;
    livekit::TrackSource map(TrackSource in) const;
    TrackInfo map(const livekit::TrackInfo& in) const;
    livekit::TrackInfo map(const TrackInfo& in) const;
    VideoQuality map(livekit::VideoQuality in) const;
    livekit::VideoQuality map(VideoQuality in) const;
    VideoLayer map(const livekit::VideoLayer& in) const;
    livekit::VideoLayer map(const VideoLayer& in) const;
    TrackType map(livekit::TrackType in) const;
    livekit::TrackType map(TrackType in) const;
    SimulcastCodecInfo map(const livekit::SimulcastCodecInfo& in) const;
    livekit::SimulcastCodecInfo map(const SimulcastCodecInfo& in) const;
    BackupCodecPolicy map(livekit::BackupCodecPolicy in) const;
    livekit::BackupCodecPolicy map(BackupCodecPolicy in) const;
    EncryptionType map(livekit::Encryption_Type in) const;
    livekit::Encryption_Type map(EncryptionType in) const;
    AudioTrackFeature map(livekit::AudioTrackFeature in) const;
    livekit::AudioTrackFeature map(AudioTrackFeature in) const;
    ClientConfigSetting map(livekit::ClientConfigSetting in) const;
    ClientConfiguration map(const livekit::ClientConfiguration& in) const;
    DisabledCodecs map(const livekit::DisabledCodecs& in) const;
    VideoConfiguration map(const livekit::VideoConfiguration& in) const;
    ServerEdition map(livekit::ServerInfo_Edition in) const;
    ServerInfo map(const livekit::ServerInfo& in) const;
    SignalTarget map(livekit::SignalTarget in) const;
    livekit::SignalTarget map(SignalTarget in) const;
    LeaveRequestAction map(livekit::LeaveRequest_Action in) const;
    livekit::LeaveRequest_Action map(LeaveRequestAction in) const;
    RegionInfo map(const livekit::RegionInfo& in) const;
    livekit::RegionInfo map(const RegionInfo& in) const;
    RegionSettings map(const livekit::RegionSettings& in) const;
    livekit::RegionSettings map(const RegionSettings& in) const;
    SpeakerInfo map(const livekit::SpeakerInfo& in) const;
    ConnectionQuality map(livekit::ConnectionQuality in) const;
    ConnectionQualityInfo map(const livekit::ConnectionQualityInfo& in) const;
    StreamState map(livekit::StreamState in) const;
    StreamStateInfo map(const livekit::StreamStateInfo& in) const;
    SubscribedQuality map(const livekit::SubscribedQuality& in) const;
    SubscribedCodec map(const livekit::SubscribedCodec& in) const;
    ICEServer map(const livekit::ICEServer& in) const;
    RequestResponseReason map(livekit::RequestResponse_Reason in) const;
    SubscriptionError map(livekit::SubscriptionError in) const;
    SimulcastCodec map(const livekit::SimulcastCodec& in) const;
    livekit::SimulcastCodec map(const SimulcastCodec& in) const;
    ParticipantTracks map(const livekit::ParticipantTracks& in) const;
    livekit::ParticipantTracks map(const ParticipantTracks& in) const;
    TrackPermission map(const livekit::TrackPermission& in) const;
    livekit::TrackPermission map(const TrackPermission& in) const;
    DataChannelInfo map(const livekit::DataChannelInfo& in) const;
    livekit::DataChannelInfo map(const DataChannelInfo& in) const;
    CandidateProtocol map(livekit::CandidateProtocol in) const;
    livekit::CandidateProtocol map(CandidateProtocol in) const;
    livekit::ClientInfo_SDK map(SDK sdk) const;
    SDK map(livekit::ClientInfo_SDK sdk) const;
protected:
    // overrides of Bricks::LoggableR
    std::string_view logCategory() const final;
private:
    // helpers
    template<typename T>
    const T& map(const T& in) const { return in; }
    template <typename TOut, typename TIn = TOut, class TProtoBufRepeated>
    std::vector<TOut> rconv(const TProtoBufRepeated& in) const;
    template <typename TCppRepeated, class TProtoBufRepeated>
    void rconv(const TCppRepeated& from, TProtoBufRepeated* to) const;
    template<typename K, typename V>
    std::unordered_map<K, V> mconv(const google::protobuf::Map<K, V>& in) const;
    template<typename K, typename V>
    void mconv(const std::unordered_map<K, V>& from, google::protobuf::Map<K, V>* to) const;
};

template <typename TProto>
inline std::optional<TProto> ProtoMarshaller::fromBytes(const void* data,
                                                        size_t dataLen) const {
    if (data && dataLen) {
        TProto proto;
        if (proto.ParseFromArray(data, int(dataLen))) {
            return proto;
        }
        logError(std::string("failed parse of ") + proto.GetTypeName() + " from blob");
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
