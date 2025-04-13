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
#include "livekit/signaling/sfu/ConnectionQualityUpdate.h"
#include "livekit/signaling/sfu/JoinResponse.h"
#include "livekit/signaling/sfu/SessionDescription.h"
#include "livekit/signaling/sfu/LeaveRequest.h"
#include "livekit/signaling/sfu/MuteTrackRequest.h"
#include "livekit/signaling/sfu/TrickleRequest.h"
#include "livekit/signaling/sfu/ParticipantUpdate.h"
#include "livekit/signaling/sfu/RoomUpdate.h"
#include "livekit/signaling/sfu/SpeakersChanged.h"
#include "livekit/signaling/sfu/StreamStateUpdate.h"
#include "livekit/signaling/sfu/TrackPublishedResponse.h"
#include "livekit/signaling/sfu/TrackUnpublishedResponse.h"
#include "livekit/signaling/sfu/SubscribedQualityUpdate.h"
#include "livekit/signaling/sfu/ReconnectResponse.h"
#include "livekit/signaling/sfu/TrackSubscribed.h"
#include "livekit/signaling/sfu/RequestResponse.h"
#include "livekit/signaling/sfu/SubscriptionResponse.h"
#include "livekit/signaling/sfu/SubscriptionPermissionUpdate.h"
#include "livekit/signaling/sfu/AddTrackRequest.h"
#include "livekit/signaling/sfu/UpdateSubscription.h"
#include "livekit/signaling/sfu/UpdateTrackSettings.h"
#include "livekit/signaling/sfu/UpdateVideoLayers.h"
#include "livekit/signaling/sfu/SubscriptionPermission.h"
#include "livekit/signaling/sfu/SyncState.h"
#include "livekit/signaling/sfu/SimulateScenario.h"
#include "livekit/signaling/sfu/UpdateParticipantMetadata.h"
#include "livekit/signaling/sfu/Ping.h"
#include "livekit/signaling/sfu/Pong.h"
#include "livekit/signaling/sfu/UpdateLocalAudioTrack.h"
#include "livekit/signaling/sfu/UpdateLocalVideoTrack.h"
#include "livekit/signaling/sfu/ClientInfo.h"
#include "livekit/signaling/sfu/DataPacket.h"
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
    // responses & requests
    JoinResponse map(livekit::JoinResponse in) const;
    SessionDescription map(livekit::SessionDescription in) const;
    livekit::SessionDescription map(SessionDescription in) const;
    TrickleRequest map(livekit::TrickleRequest in) const;
    livekit::TrickleRequest map(TrickleRequest in) const;
    ParticipantUpdate map(livekit::ParticipantUpdate in) const;
    TrackPublishedResponse map(livekit::TrackPublishedResponse in) const;
    livekit::TrackPublishedResponse map(TrackPublishedResponse in) const;
    TrackUnpublishedResponse map(livekit::TrackUnpublishedResponse in) const;
    LeaveRequest map(livekit::LeaveRequest in) const;
    livekit::LeaveRequest map(LeaveRequest in) const;
    MuteTrackRequest map(livekit::MuteTrackRequest in) const;
    livekit::MuteTrackRequest map(MuteTrackRequest in) const;
    SpeakersChanged map(livekit::SpeakersChanged in) const;
    RoomUpdate map(livekit::RoomUpdate in) const;
    ConnectionQualityUpdate map(livekit::ConnectionQualityUpdate in) const;
    StreamStateUpdate map(livekit::StreamStateUpdate in) const;
    SubscribedQualityUpdate map(livekit::SubscribedQualityUpdate in) const;
    ReconnectResponse map(livekit::ReconnectResponse in) const;
    TrackSubscribed map(livekit::TrackSubscribed in) const;
    RequestResponse map(livekit::RequestResponse in) const;
    SubscriptionResponse map(livekit::SubscriptionResponse in) const;
    SubscriptionPermissionUpdate map(livekit::SubscriptionPermissionUpdate in) const;
    AddTrackRequest map(livekit::AddTrackRequest in) const;
    livekit::AddTrackRequest map(AddTrackRequest in) const;
    UpdateSubscription map(livekit::UpdateSubscription in) const;
    livekit::UpdateSubscription map(UpdateSubscription in) const;
    UpdateTrackSettings map(livekit::UpdateTrackSettings in) const;
    livekit::UpdateTrackSettings map(UpdateTrackSettings in) const;
    UpdateVideoLayers map(livekit::UpdateVideoLayers in) const;
    livekit::UpdateVideoLayers map(UpdateVideoLayers in) const;
    SubscriptionPermission map(livekit::SubscriptionPermission in) const;
    livekit::SubscriptionPermission map(SubscriptionPermission in) const;
    SyncState map(livekit::SyncState in) const;
    livekit::SyncState map(SyncState in) const;
    SimulateScenario map(livekit::SimulateScenario in) const;
    livekit::SimulateScenario map(SimulateScenario in) const;
    UpdateParticipantMetadata map(livekit::UpdateParticipantMetadata in) const;
    livekit::UpdateParticipantMetadata map(UpdateParticipantMetadata in) const;
    Ping map(livekit::Ping in) const;
    livekit::Ping map(Ping in) const;
    Pong map(livekit::Pong in) const;
    livekit::Pong map(Pong in) const;
    UpdateLocalAudioTrack map(livekit::UpdateLocalAudioTrack in) const;
    livekit::UpdateLocalAudioTrack map(UpdateLocalAudioTrack in) const;
    UpdateLocalVideoTrack map(livekit::UpdateLocalVideoTrack in) const;
    livekit::UpdateLocalVideoTrack map(UpdateLocalVideoTrack in) const;
    ClientInfo map(livekit::ClientInfo in) const;
    livekit::ClientInfo map(ClientInfo in) const;
    // data
    RoomInfo map(livekit::Room in) const;
    Codec map(livekit::Codec in) const;
    TimedVersion map(livekit::TimedVersion in) const;
    livekit::TimedVersion map(TimedVersion in) const;
    ParticipantInfo map(livekit::ParticipantInfo in) const;
    ParticipantKind map(livekit::ParticipantInfo_Kind in) const;
    ParticipantState map(livekit::ParticipantInfo_State in) const;
    ParticipantPermission map(livekit::ParticipantPermission in) const;
    DisconnectReason map(livekit::DisconnectReason in) const;
    livekit::DisconnectReason map(DisconnectReason in) const;
    TrackSource map(livekit::TrackSource in) const;
    livekit::TrackSource map(TrackSource in) const;
    TrackInfo map(livekit::TrackInfo in) const;
    livekit::TrackInfo map(TrackInfo in) const;
    VideoQuality map(livekit::VideoQuality in) const;
    livekit::VideoQuality map(VideoQuality in) const;
    VideoLayer map(livekit::VideoLayer in) const;
    livekit::VideoLayer map(VideoLayer in) const;
    TrackType map(livekit::TrackType in) const;
    livekit::TrackType map(TrackType in) const;
    SimulcastCodecInfo map(livekit::SimulcastCodecInfo in) const;
    livekit::SimulcastCodecInfo map(SimulcastCodecInfo in) const;
    BackupCodecPolicy map(livekit::BackupCodecPolicy in) const;
    livekit::BackupCodecPolicy map(BackupCodecPolicy in) const;
    EncryptionType map(livekit::Encryption_Type in) const;
    livekit::Encryption_Type map(EncryptionType in) const;
    AudioTrackFeature map(livekit::AudioTrackFeature in) const;
    livekit::AudioTrackFeature map(AudioTrackFeature in) const;
    ClientConfigSetting map(livekit::ClientConfigSetting in) const;
    ClientConfiguration map(livekit::ClientConfiguration in) const;
    DisabledCodecs map(livekit::DisabledCodecs in) const;
    VideoConfiguration map(livekit::VideoConfiguration in) const;
    ServerEdition map(livekit::ServerInfo_Edition in) const;
    ServerInfo map(livekit::ServerInfo in) const;
    SignalTarget map(livekit::SignalTarget in) const;
    livekit::SignalTarget map(SignalTarget in) const;
    LeaveRequestAction map(livekit::LeaveRequest_Action in) const;
    livekit::LeaveRequest_Action map(LeaveRequestAction in) const;
    RegionInfo map(livekit::RegionInfo in) const;
    livekit::RegionInfo map(RegionInfo in) const;
    RegionSettings map(livekit::RegionSettings in) const;
    livekit::RegionSettings map(RegionSettings in) const;
    SpeakerInfo map(livekit::SpeakerInfo in) const;
    ConnectionQuality map(livekit::ConnectionQuality in) const;
    ConnectionQualityInfo map(livekit::ConnectionQualityInfo in) const;
    StreamState map(livekit::StreamState in) const;
    StreamStateInfo map(livekit::StreamStateInfo in) const;
    SubscribedQuality map(livekit::SubscribedQuality in) const;
    SubscribedCodec map(livekit::SubscribedCodec in) const;
    ICEServer map(livekit::ICEServer in) const;
    RequestResponseReason map(livekit::RequestResponse_Reason in) const;
    SubscriptionError map(livekit::SubscriptionError in) const;
    SimulcastCodec map(livekit::SimulcastCodec in) const;
    livekit::SimulcastCodec map(SimulcastCodec in) const;
    ParticipantTracks map(livekit::ParticipantTracks in) const;
    livekit::ParticipantTracks map(ParticipantTracks in) const;
    TrackPermission map(livekit::TrackPermission in) const;
    livekit::TrackPermission map(TrackPermission in) const;
    DataChannelInfo map(livekit::DataChannelInfo in) const;
    livekit::DataChannelInfo map(DataChannelInfo in) const;
    CandidateProtocol map(livekit::CandidateProtocol in) const;
    livekit::CandidateProtocol map(CandidateProtocol in) const;
    livekit::ClientInfo_SDK map(SDK sdk) const;
    SDK map(livekit::ClientInfo_SDK sdk) const;
    livekit::DataPacket map(DataPacket in) const;
    DataPacket map(livekit::DataPacket in) const;
    livekit::DataPacket::Kind map(DataPacketKind kind) const;
    DataPacketKind map(livekit::DataPacket::Kind kind) const;
    livekit::UserPacket map(UserPacket in) const;
    UserPacket map(livekit::UserPacket in) const;
    livekit::ChatMessage map(ChatMessage in) const;
    ChatMessage map(livekit::ChatMessage in) const;
    livekit::DataStream::OperationType map(DataStreamTextHeaderOperationType type) const;
    DataStreamTextHeaderOperationType map(livekit::DataStream::OperationType type) const;
    livekit::DataStream::ByteHeader map(DataStreamByteHeader in) const;
    DataStreamByteHeader map(livekit::DataStream::ByteHeader in) const;
    livekit::DataStream::Chunk map(DataStreamChunk in) const;
    DataStreamChunk map(livekit::DataStream::Chunk in) const;
    livekit::DataStream::TextHeader map(DataStreamTextHeader in) const;
    DataStreamTextHeader map(livekit::DataStream::TextHeader in) const;
    livekit::DataStream::Header map(DataStreamHeader in) const;
    DataStreamHeader map(livekit::DataStream::Header in) const;
    livekit::DataStream::Trailer map(DataStreamTrailer in) const;
    DataStreamTrailer map(livekit::DataStream::Trailer in) const;
protected:
    // overrides of Bricks::LoggableR
    std::string_view logCategory() const final;
private:
    // helpers
    template <typename T>
    T map(T in) const { return std::move(in); }
    template <typename TOut, typename TIn = TOut, class TProtoBufRepeated>
    std::vector<TOut> rconv(TProtoBufRepeated in) const;
    template <typename TCppRepeated, class TProtoBufRepeated>
    void rconv(TCppRepeated from, TProtoBufRepeated* to) const;
    template <typename K, typename V>
    std::unordered_map<K, V> mconv(google::protobuf::Map<K, V> in) const;
    template <typename K, typename V>
    void mconv(std::unordered_map<K, V> from, google::protobuf::Map<K, V>* to) const;
};

} // namespace LiveKitCpp
