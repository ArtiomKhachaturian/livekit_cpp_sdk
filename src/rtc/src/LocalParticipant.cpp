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
#include "LocalParticipant.h"
#include "AdmProxyFacade.h"
#include "AudioDeviceImpl.h"
//#include "Blob.h"
#include "LocalVideoDeviceImpl.h"
#include "TransportManager.h"
//#include "DataChannel.h"
#include "RtcUtils.h"
#include "PeerConnectionFactory.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"

#include <iostream>

namespace LiveKitCpp
{

LocalParticipant::LocalParticipant(PeerConnectionFactory* pcf,
                                   const Participant* session,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
    , _pcf(pcf)
    , _session(session)
{
}

void LocalParticipant::reset()
{
    _session(nullptr);
}

std::shared_ptr<AudioDeviceImpl> LocalParticipant::addDevice(std::unique_ptr<AudioDevice> device,
                                                             EncryptionType encryption)
{
    if (auto impl = dynamic_cast<AudioDeviceImpl*>(device.release())) {
        auto id = impl->id();
        if (!id.empty()) {
            LOCK_WRITE_SAFE_OBJ(_audioDevices);
            if (!_audioDevices->count(id)) {
                std::shared_ptr<AudioDeviceImpl> wrapper(impl);
                _audioDevices->insert(std::make_pair(std::move(id), std::make_pair(wrapper, encryption)));
                return wrapper;
            }
        }
    }
    return {};
}

std::shared_ptr<LocalVideoDeviceImpl> LocalParticipant::addDevice(std::unique_ptr<LocalVideoDevice> device,
                                                                  EncryptionType encryption)
{
    if (auto impl = dynamic_cast<LocalVideoDeviceImpl*>(device.release())) {
        auto id = impl->id();
        if (!id.empty()) {
            LOCK_WRITE_SAFE_OBJ(_videoDevices);
            if (!_videoDevices->count(id)) {
                std::shared_ptr<LocalVideoDeviceImpl> wrapper(impl);
                _videoDevices->insert(std::make_pair(std::move(id), std::make_pair(wrapper, encryption)));
                return wrapper;
            }
        }
    }
    return {};
}

bool LocalParticipant::removeDevice(const std::string& id)
{
    return removeAudioDevice(id) || removeVideoDevice(id);
}

size_t LocalParticipant::devicesCount() const
{
    LOCK_READ_SAFE_OBJ(_audioDevices);
    LOCK_READ_SAFE_OBJ(_videoDevices);
    return _audioDevices->size() + _videoDevices->size();
}

void LocalParticipant::addDevicesToTransportManager(TransportManager* manager) const
{
    addAudioDevicesToTransportManager(manager);
    addVideoDevicesToTransportManager(manager);
}

std::optional<bool> LocalParticipant::stereoRecording() const
{
    if (_pcf) {
        if (const auto admProxy = _pcf->admProxy().lock()) {
            return admProxy->recordingState().stereo();
        }
    }
    return std::nullopt;
}

/*std::shared_ptr<LocalAudioTrackImpl> LocalParticipant::addAudioTrack(std::shared_ptr<AudioDevice> device,
                                                                     EncryptionType encryption,
                                                                     const std::weak_ptr<TrackManager>& trackManager)
{
    if (auto audio = std::dynamic_pointer_cast<AudioDeviceImpl>(device)) {
        auto track = std::make_shared<LocalAudioTrackImpl>(encryption, std::move(audio),  trackManager, true);
        addTrack(track, _audioTracks);
        return track;
    }
    return {};
}

std::shared_ptr<LocalVideoTrackImpl> LocalParticipant::addVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                                     EncryptionType encryption,
                                                                     const std::weak_ptr<TrackManager>& trackManager)
{
    if (auto video = std::dynamic_pointer_cast<LocalVideoDeviceImpl>(device)) {
        auto track = std::make_shared<LocalVideoTrackImpl>(encryption, std::move(video),  trackManager);
        addTrack(track, _videoTracks);
        return track;
    }
    return {};
}*/

void LocalParticipant::setInfo(const ParticipantInfo& info)
{
    if (exchangeVal(info._sid, _sid)) {
        notify(&ParticipantListener::onSidChanged);
    }
    if (exchangeVal(info._identity, _identity)) {
        notify(&ParticipantListener::onIdentityChanged);
    }
    if (exchangeVal(info._name, _name)) {
        notify(&ParticipantListener::onNameChanged);
    }
    if (exchangeVal(info._metadata, _metadata)) {
        notify(&ParticipantListener::onMetadataChanged);
    }
    if (exchangeVal(info._kind, _kind)) {
        notify(&ParticipantListener::onKindChanged);
    }
}

void LocalParticipant::setSpeakerChanges(float level, bool active) const
{
    notify(&ParticipantListener::onSpeakerInfoChanged, level, active);
}

void LocalParticipant::setConnectionQuality(ConnectionQuality quality,
                                                           float score)
{
    notify(&ParticipantListener::onConnectionQualityChanged, quality, score);
}

bool LocalParticipant::removeAudioDevice(const std::string& id)
{
    if (!id.empty()) {
        LOCK_WRITE_SAFE_OBJ(_audioDevices);
        return _audioDevices->erase(id);
    }
    return false;
}

bool LocalParticipant::removeVideoDevice(const std::string& id)
{
    if (!id.empty()) {
        LOCK_WRITE_SAFE_OBJ(_videoDevices);
        return _videoDevices->erase(id);
    }
    return false;
}

void LocalParticipant::addAudioDevicesToTransportManager(TransportManager* manager) const
{
    if (manager) {
        LOCK_READ_SAFE_OBJ(_audioDevices);
        for (auto it = _audioDevices->begin(); it != _audioDevices->end(); ++it) {
            manager->addTrack(it->second.first, it->second.second);
        }
    }
}

void LocalParticipant::addVideoDevicesToTransportManager(TransportManager* manager) const
{
    if (manager) {
        LOCK_READ_SAFE_OBJ(_videoDevices);
        for (auto it = _videoDevices->begin(); it != _videoDevices->end(); ++it) {
            manager->addTrack(it->second.first, it->second.second);
        }
    }
}

template <class Method, typename... Args>
void LocalParticipant::notify(const Method& method, Args&&... args) const
{
    LOCK_READ_SAFE_OBJ(_session);
    if (const auto session = _session.constRef()) {
        _listener.invoke(method, session, std::forward<Args>(args)...);
    }
}

void LocalParticipant::onEncryptionStateChanged(webrtc::MediaType mediaType,
                                                const std::string&,
                                                const std::string& trackId,
                                                AesCgmCryptorState state)
{
    if (const auto err = toCryptoError(state)) {
        notify(&ParticipantListener::onTrackCryptoError,
               mediaTypeToTrackType(mediaType),
               EncryptionType::Gcm, trackId, err.value());
    }
}

} // namespace LiveKitCpp
