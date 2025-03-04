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
#include "rtc/UpdateLocalAudioTrack.h"
#include "rtc/UpdateLocalVideoTrack.h"
#include "livekit_rtc.pb.h"
#include <optional>
#include <unordered_map>

namespace LiveKitCpp
{

class Signals
{
public:
    // responses & requests
    static JoinResponse map(const livekit::JoinResponse& in);
    static SessionDescription map(const livekit::SessionDescription& in);
    static livekit::SessionDescription map(const SessionDescription& in);
    static TrickleRequest map(const livekit::TrickleRequest& in);
    static livekit::TrickleRequest map(const TrickleRequest& in);
    static ParticipantUpdate map(const livekit::ParticipantUpdate& in);
    static TrackPublishedResponse map(const livekit::TrackPublishedResponse& in);
    static livekit::TrackPublishedResponse map(const TrackPublishedResponse& in);
    static TrackUnpublishedResponse map(const livekit::TrackUnpublishedResponse& in);
    static LeaveRequest map(const livekit::LeaveRequest& in);
    static livekit::LeaveRequest map(const LeaveRequest& in);
    static MuteTrackRequest map(const livekit::MuteTrackRequest& in);
    static livekit::MuteTrackRequest map(const MuteTrackRequest& in);
    static SpeakersChanged map(const livekit::SpeakersChanged& in);
    static RoomUpdate map(const livekit::RoomUpdate& in);
    static ConnectionQualityUpdate map(const livekit::ConnectionQualityUpdate& in);
    static StreamStateUpdate map(const livekit::StreamStateUpdate& in);
    static SubscribedQualityUpdate map(const livekit::SubscribedQualityUpdate& in);
    static ReconnectResponse map(const livekit::ReconnectResponse& in);
    static TrackSubscribed map(const livekit::TrackSubscribed& in);
    static RequestResponse map(const livekit::RequestResponse& in);
    static SubscriptionResponse map(const livekit::SubscriptionResponse& in);
    static SubscriptionPermissionUpdate map(const livekit::SubscriptionPermissionUpdate& in);
    static AddTrackRequest map(const livekit::AddTrackRequest& in);
    static livekit::AddTrackRequest map(const AddTrackRequest& in);
    static UpdateSubscription map(const livekit::UpdateSubscription& in);
    static livekit::UpdateSubscription map(const UpdateSubscription& in);
    static UpdateTrackSettings map(const livekit::UpdateTrackSettings& in);
    static livekit::UpdateTrackSettings map(const UpdateTrackSettings& in);
    static UpdateVideoLayers map(const livekit::UpdateVideoLayers& in);
    static livekit::UpdateVideoLayers map(const UpdateVideoLayers& in);
    static SubscriptionPermission map(const livekit::SubscriptionPermission& in);
    static livekit::SubscriptionPermission map(const SubscriptionPermission& in);
    static SyncState map(const livekit::SyncState& in);
    static livekit::SyncState map(const SyncState& in);
    static SimulateScenario map(const livekit::SimulateScenario& in);
    static livekit::SimulateScenario map(const SimulateScenario& in);
    static UpdateParticipantMetadata map(const livekit::UpdateParticipantMetadata& in);
    static livekit::UpdateParticipantMetadata map(const UpdateParticipantMetadata& in);
    static Ping map(const livekit::Ping& in);
    static livekit::Ping map(const Ping& in);
    static UpdateLocalAudioTrack map(const livekit::UpdateLocalAudioTrack& in);
    static livekit::UpdateLocalAudioTrack map(const UpdateLocalAudioTrack& in);
    static UpdateLocalVideoTrack map(const livekit::UpdateLocalVideoTrack& in);
    static livekit::UpdateLocalVideoTrack map(const UpdateLocalVideoTrack& in);
    // data
    static Room map(const livekit::Room& in);
    static Codec map(const livekit::Codec& in);
    static TimedVersion map(const livekit::TimedVersion& in);
    static livekit::TimedVersion map(const TimedVersion& in);
    static ParticipantInfo map(const livekit::ParticipantInfo& in);
    static ParticipantKind map(livekit::ParticipantInfo_Kind in);
    static ParticipantState map(livekit::ParticipantInfo_State in);
    static ParticipantPermission map(const livekit::ParticipantPermission& in);
    static DisconnectReason map(livekit::DisconnectReason in);
    static livekit::DisconnectReason map(DisconnectReason in);
    static TrackSource map(livekit::TrackSource in);
    static livekit::TrackSource map(TrackSource in);
    static TrackInfo map(const livekit::TrackInfo& in);
    static livekit::TrackInfo map(const TrackInfo& in);
    static VideoQuality map(livekit::VideoQuality in);
    static livekit::VideoQuality map(VideoQuality in);
    static VideoLayer map(const livekit::VideoLayer& in);
    static livekit::VideoLayer map(const VideoLayer& in);
    static TrackType map(livekit::TrackType in);
    static livekit::TrackType map(TrackType in);
    static SimulcastCodecInfo map(const livekit::SimulcastCodecInfo& in);
    static livekit::SimulcastCodecInfo map(const SimulcastCodecInfo& in);
    static BackupCodecPolicy map(livekit::BackupCodecPolicy in);
    static livekit::BackupCodecPolicy map(BackupCodecPolicy in);
    static EncryptionType map(livekit::Encryption_Type in);
    static livekit::Encryption_Type map(EncryptionType in);
    static AudioTrackFeature map(livekit::AudioTrackFeature in);
    static livekit::AudioTrackFeature map(AudioTrackFeature in);
    static ClientConfigSetting map(livekit::ClientConfigSetting in);
    static ClientConfiguration map(const livekit::ClientConfiguration& in);
    static DisabledCodecs map(const livekit::DisabledCodecs& in);
    static VideoConfiguration map(const livekit::VideoConfiguration& in);
    static ServerEdition map(livekit::ServerInfo_Edition in);
    static ServerInfo map(const livekit::ServerInfo& in);
    static SignalTarget map(livekit::SignalTarget in);
    static livekit::SignalTarget map(SignalTarget in);
    static LeaveRequestAction map(livekit::LeaveRequest_Action in);
    static livekit::LeaveRequest_Action map(LeaveRequestAction in);
    static RegionInfo map(const livekit::RegionInfo& in);
    static livekit::RegionInfo map(const RegionInfo& in);
    static RegionSettings map(const livekit::RegionSettings& in);
    static livekit::RegionSettings map(const RegionSettings& in);
    static SpeakerInfo map(const livekit::SpeakerInfo& in);
    static ConnectionQuality map(livekit::ConnectionQuality in);
    static ConnectionQualityInfo map(const livekit::ConnectionQualityInfo& in);
    static StreamState map(livekit::StreamState in);
    static StreamStateInfo map(const livekit::StreamStateInfo& in);
    static SubscribedQuality map(const livekit::SubscribedQuality& in);
    static SubscribedCodec map(const livekit::SubscribedCodec& in);
    static ICEServer map(const livekit::ICEServer& in);
    static RequestResponseReason map(livekit::RequestResponse_Reason in);
    static SubscriptionError map(livekit::SubscriptionError in);
    static SimulcastCodec map(const livekit::SimulcastCodec& in);
    static livekit::SimulcastCodec map(const SimulcastCodec& in);
    static ParticipantTracks map(const livekit::ParticipantTracks& in);
    static livekit::ParticipantTracks map(const ParticipantTracks& in);
    static TrackPermission map(const livekit::TrackPermission& in);
    static livekit::TrackPermission map(const TrackPermission& in);
    static DataChannelInfo map(const livekit::DataChannelInfo& in);
    static livekit::DataChannelInfo map(const DataChannelInfo& in);
    static CandidateProtocol map(livekit::CandidateProtocol in);
    static livekit::CandidateProtocol map(CandidateProtocol in);
private:
    // helpers
    template<typename T>
    static const T& map(const T& in) { return in; }
    template <typename TOut, typename TIn = TOut, class TProtoBufRepeated>
    static std::vector<TOut> rconv(const TProtoBufRepeated& in);
    template <typename TCppRepeated, class TProtoBufRepeated>
    static void rconv(const TCppRepeated& from, TProtoBufRepeated* to);
    template<typename K, typename V>
    static std::unordered_map<K, V> mconv(const google::protobuf::Map<K, V>& in);
    template<typename K, typename V>
    static void mconv(const std::unordered_map<K, V>& from, google::protobuf::Map<K, V>* to);
};

} // namespace LiveKitCpp
