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
#include "ParticipantImpl.h"
#include "CameraTrackImpl.h"
#include "LocalAudioTrackImpl.h"
#include "LocalParticipant.h"
#include "LocalTrackManager.h"
#include "DataChannelsStorage.h"
#include "SafeObj.h"
#include <vector>
#include <unordered_map>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class LocalParticipantImpl : public ParticipantImpl<LocalParticipant>,
                             protected DataChannelsStorage<LocalTrackManager>
{
    // key is cid (track id), for LocalTrackManager [publishMedia] / [unpublishMedia]
    using PendingLocalMedias = std::unordered_map<std::string, webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>;
public:
    LocalParticipantImpl(LocalTrackManager* manager, const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipantImpl() final;
    bool addDataChannel(rtc::scoped_refptr<DataChannel> channel);
    void addTracksToTransport();
    void resetTracksMedia();
    LocalTrack* track(const std::string& id, bool cid);
    LocalTrack* track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> pendingLocalMedia();
    // impl. of LocalParticipant
    CameraTrackImpl& camera() final { return _camera; }
    const CameraTrackImpl& camera() const final { return _camera; }
    LocalAudioTrackImpl& microphone() final { return _microphone; }
    const LocalAudioTrackImpl& microphone() const final { return _microphone; }
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    // impl. of LocalTrackManager
    bool addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) final;
    bool removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) final;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createMic(const std::string& label) final;
    webrtc::scoped_refptr<CameraVideoTrack> createCamera(const std::string& label) final;
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
private:
    LocalTrackManager* const _manager;
    LocalAudioTrackImpl _microphone;
    CameraTrackImpl _camera;
    Bricks::SafeObj<PendingLocalMedias> _pendingLocalMedias;
};

} // namespace LiveKitCpp
