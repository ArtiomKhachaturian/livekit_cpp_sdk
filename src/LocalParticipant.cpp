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
#ifdef WEBRTC_AVAILABLE
#include "LocalParticipant.h"
#include "Blob.h"
#include "CameraVideoTrack.h"
#include "CameraVideoSource.h"
#include "CameraManager.h"
#include "DataChannel.h"
#include "MicrophoneOptions.h"
#include "Utils.h"
#include "PeerConnectionFactory.h"
#include "rtc/ParticipantInfo.h"

namespace {

using namespace LiveKitCpp;

inline cricket::AudioOptions toCricketOptions(const MicrophoneOptions& options) {
    cricket::AudioOptions audioOptions;
    audioOptions.echo_cancellation = options._echoCancellation;
    audioOptions.auto_gain_control = options._autoGainControl;
    audioOptions.noise_suppression = options._noiseSuppression;
    audioOptions.highpass_filter = options._highpassFilter;
    audioOptions.stereo_swapping = options._stereoSwapping;
    return audioOptions;
}

}

namespace LiveKitCpp
{

LocalParticipant::LocalParticipant(TrackManager* manager,
                                   PeerConnectionFactory* pcf,
                                   const Participant* session,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
    , _manager(manager)
    , _pcf(pcf)
    , _session(session)
{
}

void LocalParticipant::reset()
{
    _session(nullptr);
    _micTrack({});
    _audioTracks({});
    _videoTracks({});
}

std::optional<bool> LocalParticipant::stereoRecording() const
{
    if (_pcf) {
        return _pcf->stereoRecording();
    }
    return std::nullopt;
}

size_t LocalParticipant::audioTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    return _audioTracks->size();
}

size_t LocalParticipant::videoTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    return _videoTracks->size();
}

std::shared_ptr<LocalAudioTrackImpl> LocalParticipant::
    addMicrophoneTrack(const MicrophoneOptions& options)
{
    LOCK_WRITE_SAFE_OBJ(_micTrack);
    if (!_micTrack.constRef()) {
        if (auto mic = createMic(options)) {
            _micTrack = std::make_shared<LocalAudioTrackImpl>(std::move(mic),
                                                              _manager, true,
                                                              logger());
            addTrack(_micTrack.constRef(), _audioTracks);
        }
    }
    return _micTrack.constRef();
}

std::shared_ptr<CameraTrackImpl> LocalParticipant::
    addCameraTrack(const CameraOptions& options)
{
    if (auto camera = createCamera(options)) {
        auto track = std::make_shared<CameraTrackImpl>(std::move(camera),
                                                       _manager, logger());
        addTrack(track, _videoTracks);
        return track;
    }
    return {};
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalParticipant::
    removeAudioTrack(const std::shared_ptr<AudioTrack>& track)
{
    if (const auto local = std::dynamic_pointer_cast<LocalAudioTrackImpl>(track)) {
        {
            LOCK_WRITE_SAFE_OBJ(_micTrack);
            if (_micTrack.constRef() == local) {
                _micTrack = {};
            }
        }
        LOCK_WRITE_SAFE_OBJ(_audioTracks);
        for (auto it = _audioTracks->begin(); it != _audioTracks->end(); ++it) {
            if (*it == local) {
                _audioTracks->erase(it);
                return local->media();
            }
        }
    }
    return {};
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalParticipant::
    removeVideoTrack(const std::shared_ptr<VideoTrack>& track)
{
    if (const auto local = std::dynamic_pointer_cast<LocalTrack>(track)) {
        LOCK_WRITE_SAFE_OBJ(_videoTracks);
        for (auto it = _videoTracks->begin(); it != _videoTracks->end(); ++it) {
            if (*it == track) {
                _videoTracks->erase(it);
                return local->media();
            }
        }
    }
    return {};
}

std::shared_ptr<AudioTrack> LocalParticipant::audioTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    if (index < _audioTracks->size()) {
        return _audioTracks->at(index);
    }
    return {};
}

std::shared_ptr<VideoTrack> LocalParticipant::videoTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    if (index < _videoTracks->size()) {
        return _videoTracks->at(index);
    }
    return {};
}

std::vector<std::shared_ptr<LocalTrack>> LocalParticipant::tracks() const
{
    std::vector<std::shared_ptr<LocalTrack>> tracks;
    LOCK_READ_SAFE_OBJ(_audioTracks);
    LOCK_READ_SAFE_OBJ(_videoTracks);
    tracks.reserve(_audioTracks->size() + _videoTracks->size());
    for (const auto& track : _audioTracks.constRef()) {
        if (auto localTrack = std::dynamic_pointer_cast<LocalTrack>(track)) {
            tracks.push_back(std::move(localTrack));
        }
    }
    for (const auto& track : _videoTracks.constRef()) {
        if (auto localTrack = std::dynamic_pointer_cast<LocalTrack>(track)) {
            tracks.push_back(std::move(localTrack));
        }
    }
    return tracks;
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> LocalParticipant::media() const
{
    const auto tracks = this->tracks();
    if (const auto size = tracks.size()) {
        std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> medias;
        for (const auto& track : tracks) {
            medias.push_back(track->media());
        }
        return medias;
    }
    return {};
}

std::shared_ptr<LocalTrack> LocalParticipant::track(const std::string& id, bool cid,
                                                    const std::optional<cricket::MediaType>& hint) const
{
    std::shared_ptr<LocalTrack> result;
    if (!id.empty()) {
        if (hint.has_value()) {
            switch (hint.value()) {
                case cricket::MEDIA_TYPE_AUDIO:
                    result = lookup(id, cid, _audioTracks);
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
                    result = lookup(id, cid, _videoTracks);
                    break;
                default:
                    break;
            }
        }
        else {
            result = lookup(id, cid, _audioTracks);
            if (!result) {
                result = lookup(id, cid, _videoTracks);
            }
        }
    }
    return result;
}

std::shared_ptr<LocalTrack> LocalParticipant::
    track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender) {
        return track(sender->id(), true, sender->media_type());
    }
    return {};
}

void LocalParticipant::setInfo(const ParticipantInfo& info)
{
    bool changed = false;
    if (exchangeVal(info._sid, _sid)) {
        changed = true;
    }
    if (exchangeVal(info._identity, _identity)) {
        changed = true;
    }
    if (exchangeVal(info._name, _name)) {
        changed = true;
    }
    if (exchangeVal(info._metadata, _metadata)) {
        changed = true;
    }
    if (exchangeVal(info._kind, _kind)) {
        changed = true;
    }
    if (changed) {
        invoke(&ParticipantListener::onChanged);
    }
}

template<class TTracks>
std::shared_ptr<LocalTrack> LocalParticipant::lookup(const std::string& id,
                                                     bool cid,
                                                     const TTracks& tracks)
{
    if (!id.empty()) {
        const std::lock_guard guard(tracks.mutex());
        for (const auto& track : tracks.constRef()) {
            const auto localTrack = std::dynamic_pointer_cast<LocalTrack>(track);
            if (localTrack && id == (cid ? localTrack->cid() : track->sid())) {
                return localTrack;
            }
        }
    }
    return {};
}

template<class TTrack, class TTracks>
void LocalParticipant::addTrack(const std::shared_ptr<TTrack>& track, TTracks& tracks)
{
    if (track) {
        const std::lock_guard guard(tracks.mutex());
        tracks->push_back(track);
    }
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalParticipant::
    createMic(const MicrophoneOptions& options) const
{
    if (_pcf) {
        if (const auto source = _pcf->CreateAudioSource(toCricketOptions(options))) {
            return _pcf->CreateAudioTrack(makeUuid(), source.get());
        }
    }
    return {};
}

webrtc::scoped_refptr<CameraVideoTrack> LocalParticipant::
    createCamera(const CameraOptions& options) const
{
    if (_pcf && CameraManager::available()) {
        auto source = webrtc::make_ref_counted<CameraVideoSource>(_pcf->signalingThread(),
                                                                  map(options),
                                                                  logger());
        return webrtc::make_ref_counted<CameraVideoTrack>(makeUuid(),
                                                          std::move(source),
                                                          logger());
    }
    return {};
}

template <class Method, typename... Args>
void LocalParticipant::invoke(const Method& method, Args&&... args) const
{
    LOCK_READ_SAFE_OBJ(_session);
    if (const auto session = _session.constRef()) {
        _listener.invoke(method, session, std::forward<Args>(args)...);
    }
}

void LocalParticipant::onEncryptionStateChanged(cricket::MediaType mediaType,
                                                const std::string&,
                                                const std::string& trackId,
                                                AesCgmCryptorState state)
{
    if (const auto err = toCryptoError(state)) {
        invoke(&ParticipantListener::onTrackCryptoError,
               mediaTypeToTrackType(mediaType),
               EncryptionType::Gcm, trackId, err.value());
    }
}

} // namespace LiveKitCpp
#endif
