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
#pragma once // LocalParticipantImpl.h
#ifdef WEBRTC_AVAILABLE
#include "ParticipantImpl.h"
#include "CameraTrackImpl.h"
#include "LocalAudioTrackImpl.h"
#include "LocalParticipant.h"
#include "LocalTrackManager.h"
#include "DataChannelsStorage.h"
#include "ParticipantListener.h"
#include "SafeObj.h"
#include <atomic>
#include <vector>
#include <unordered_map>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

struct TrackPublishedResponse;
struct TrackUnpublishedResponse;

class LocalParticipantImpl : public ParticipantImpl<ParticipantListener, LocalParticipant>,
                             private LocalTrackManager
{
    // key is cid (track id), for LocalTrackManager [publishMedia] / [unpublishMedia]
    using PendingLocalMedias = std::unordered_map<std::string, webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>;
public:
    LocalParticipantImpl(LocalTrackManager* manager, const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipantImpl() final;
    bool addDataChannel(rtc::scoped_refptr<DataChannel> channel);
    void addTracksToTransport();
    void reset();
    void notifyThatTrackPublished(const TrackPublishedResponse& response);
    void notifyThatTrackUnpublished(const TrackUnpublishedResponse& response);
    LocalTrack* track(const std::string& id, bool cid);
    LocalTrack* track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> pendingLocalMedia();
    // impl of ParticipantImpl<>
    void setInfo(const ParticipantInfo& info) final;
    // impl. of Participant
    std::string sid() const final { return _sid(); }
    std::string identity() const final { return _identity(); }
    std::string name() const final { return _name(); }
    std::string metadata() const final { return _metadata(); }
    ParticipantKind kind() const final { return _kind; }
    // impl. of LocalParticipant
    CameraTrackImpl& camera() final { return _camera; }
    const CameraTrackImpl& camera() const final { return _camera; }
    LocalAudioTrackImpl& microphone() final { return _microphone; }
    const LocalAudioTrackImpl& microphone() const final { return _microphone; }
    bool publishData(const Bricks::Blob& data, const DataPublishOptions& options) final;
private:
    // impl. of LocalTrackManager
    bool addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) final;
    bool removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) final;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createMic(const std::string& label) final;
    webrtc::scoped_refptr<CameraVideoTrack> createCamera(const std::string& label) final;
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
private:
    LocalTrackManager* const _manager;
    DataChannelsStorage _dcs;
    LocalAudioTrackImpl _microphone;
    CameraTrackImpl _camera;
    Bricks::SafeObj<PendingLocalMedias> _pendingLocalMedias;
    Bricks::SafeObj<std::string> _sid;
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _name;
    Bricks::SafeObj<std::string> _metadata;
    std::atomic<ParticipantKind> _kind = ParticipantKind::Standard;
};

} // namespace LiveKitCpp
#endif
