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
#include "RemoteParticipantImpl.h"
#include "RemoteAudioTrackImpl.h"
#include "RemoteVideoTrackImpl.h"
#include "RtpReceiversStorage.h"
#include "E2ESecurityFactory.h"
#include "AesCgmCryptor.h"
#include "Seq.h"
#include "Utils.h"
#include "e2e/AesCgmCryptorObserver.h"
#include "livekit/rtc/e2e/E2ECryptoError.h"
#include <api/media_stream_interface.h>
#include <api/rtp_receiver_interface.h>
#include <optional>
#include <unordered_map>

namespace {

using namespace LiveKitCpp;

inline bool compareTrackInfo(const TrackInfo& l, const TrackInfo& r) {
    return l._sid == r._sid;
}

template <class TTrack>
struct MediaInterfaceType {};

template <>
struct MediaInterfaceType<RemoteAudioTrackImpl>
{
    using MediaInterface = webrtc::AudioTrackInterface;
};

template <>
struct MediaInterfaceType<RemoteVideoTrackImpl>
{
    using MediaInterface = webrtc::VideoTrackInterface;
};

template <class TMediaInterface>
struct DeviceType {};

template <>
struct DeviceType<webrtc::AudioTrackInterface>
{
    using Device = AudioDeviceImpl;
};

template <>
struct DeviceType<webrtc::VideoTrackInterface>
{
    using Device = VideoDeviceImpl;
};

template <class TMediaInterface>
inline webrtc::scoped_refptr<TMediaInterface> mediaTrack(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) {
    if (receiver) {
        if (const auto media = dynamic_cast<TMediaInterface*>(receiver->track().get())) {
            return webrtc::scoped_refptr<TMediaInterface>(media);
        }
    }
    return {};
}

template <class TTrack>
inline auto makeDevice(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) {
    using Media = typename MediaInterfaceType<TTrack>::MediaInterface;
    using Device = typename DeviceType<Media>::Device;
    if (auto track = mediaTrack<Media>(receiver)) {
        return std::make_shared<Device>(std::move(track));
    }
    return std::shared_ptr<Device>{};
}

inline bool compareTrackInfoInfo(const TrackInfo& l, const TrackInfo& r) {
    return l._sid == r._sid;
}

}

namespace LiveKitCpp
{

class RemoteParticipantImpl::ListenerImpl : public AesCgmCryptorObserver
{
public:
    ListenerImpl(RemoteParticipantImpl* owner);
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const;
    void set(RemoteParticipantListener* listener) { _listener = listener; }
    void reset();
    // impl. of AesCgmCryptorObserver
    void onDecryptionStateChanged(cricket::MediaType mediaType, const std::string&,
                                  const std::string& trackSid, AesCgmCryptorState state) final;
private:
    Bricks::SafeObj<RemoteParticipantImpl*> _owner;
    Bricks::Listener<RemoteParticipantListener*> _listener;
};

RemoteParticipantImpl::RemoteParticipantImpl(E2ESecurityFactory* securityFactory,
                                             const std::shared_ptr<RtpReceiversStorage>& receiversStorage,
                                             const ParticipantInfo& info)
    : _securityFactory(securityFactory)
    , _receiversStorage(receiversStorage)
    , _listener(std::make_shared<ListenerImpl>(this))
{
    setInfo(info);
}

void RemoteParticipantImpl::reset()
{
    clearTracks(_audioTracks);
    clearTracks(_videoTracks);
    _listener->reset();
}

bool RemoteParticipantImpl::setRemoteSideTrackMute(const std::string& trackSid, bool mute)
{
    if (!trackSid.empty()) {
        LOCK_WRITE_SAFE_OBJ(_info);
        if (auto trackInfo = findBySid(trackSid)) {
            if (trackInfo->_muted == mute) {
                return true;
            }
            trackInfo->_muted = mute;
            if (TrackType::Audio == trackInfo->_type) {
                LOCK_READ_SAFE_OBJ(_audioTracks);
                if (const auto ndx = findBySid(trackSid, _audioTracks.constRef())) {
                    _audioTracks->at(ndx.value())->setInfo(*trackInfo);
                    return true;
                }
            }
            if (TrackType::Video == trackInfo->_type) {
                LOCK_READ_SAFE_OBJ(_videoTracks);
                if (const auto ndx = findBySid(trackSid, _videoTracks.constRef())) {
                    _videoTracks->at(ndx.value())->setInfo(*trackInfo);
                    return true;
                }
            }
        }
    }
    return false;
}

std::optional<TrackType> RemoteParticipantImpl::trackType(const std::string& trackSid) const
{
    if (!trackSid.empty()) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(trackSid)) {
            return trackInfo->_type;
        }
    }
    return std::nullopt;
}

bool RemoteParticipantImpl::addAudio(const std::string& trackSid, rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (!receiver && _receiversStorage) {
        receiver = _receiversStorage->take(trackSid);
    }
    if (receiver && cricket::MEDIA_TYPE_AUDIO == receiver->media_type()) {
        return addTrack(trackSid, receiver, _audioTracks);
    }
    return false;
}

bool RemoteParticipantImpl::addVideo(const std::string& trackSid, rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (!receiver && _receiversStorage) {
        receiver = _receiversStorage->take(trackSid);
    }
    if (receiver && cricket::MEDIA_TYPE_VIDEO == receiver->media_type()) {
        return addTrack(trackSid, receiver, _videoTracks);
    }
    return false;
}

bool RemoteParticipantImpl::removeAudio(const std::string& trackSid)
{
    return removeTrack(trackSid, _audioTracks);
}

bool RemoteParticipantImpl::removeVideo(const std::string& trackSid)
{
    return removeTrack(trackSid, _videoTracks);
}

void RemoteParticipantImpl::setInfo(const ParticipantInfo& info)
{
    {
        LOCK_WRITE_SAFE_OBJ(_info);
        using SeqType = Seq<TrackInfo>;
        const auto added = SeqType::difference(info._tracks, _info->_tracks, compareTrackInfo);
        const auto removed = SeqType::difference(_info->_tracks, info._tracks, compareTrackInfo);
        const auto updated = SeqType::intersection(_info->_tracks, info._tracks, compareTrackInfo);
        _info = info;
        // add new
        for (const auto& track : added) {
            switch (track._type) {
                case TrackType::Audio:
                    addAudio(track._sid);
                    break;
                case TrackType::Video:
                    addVideo(track._sid);
                    break;
                default:
                    break;
            }
        }
        // remove
        for (const auto& track : removed) {
            switch (track._type) {
                case TrackType::Audio:
                    removeAudio(track._sid);
                    break;
                case TrackType::Video:
                    removeVideo(track._sid);
                    break;
                default:
                    break;
            }
        }
        // update existed
        for (const auto& track : updated) {
            switch (track._type) {
                case TrackType::Audio:
                    if (!updateAudio(track)) {
                        addAudio(track._sid);
                    }
                    break;
                case TrackType::Video:
                    if (!updateVideo(track)) {
                        addVideo(track._sid);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    _listener->invoke(&RemoteParticipantListener::onChanged);
}

std::string RemoteParticipantImpl::sid() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_sid;
}

std::string RemoteParticipantImpl::identity() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_identity;
}

std::string RemoteParticipantImpl::name() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_name;
}

std::string RemoteParticipantImpl::metadata() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_metadata;
}

ParticipantKind RemoteParticipantImpl::kind() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_kind;
}

void RemoteParticipantImpl::setListener(RemoteParticipantListener* listener)
{
    _listener->set(listener);
}

bool RemoteParticipantImpl::hasActivePublisher() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_isPublisher;
}

ParticipantState RemoteParticipantImpl::state() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_state;
}

size_t RemoteParticipantImpl::audioTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    return _audioTracks->size();
}

size_t RemoteParticipantImpl::videoTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    return _videoTracks->size();
}

std::shared_ptr<RemoteAudioTrack> RemoteParticipantImpl::audioTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    if (index < _audioTracks->size()) {
        return _audioTracks->at(index);
    }
    return {};
}

std::shared_ptr<RemoteAudioTrack> RemoteParticipantImpl::audioTrack(const std::string& trackSid) const
{
    if (!trackSid.empty()) {
        LOCK_READ_SAFE_OBJ(_audioTracks);
        if (const auto ndx = findBySid(trackSid, _audioTracks.constRef())) {
            return _audioTracks->at(ndx.value());
        }
    }
    return {};
}

std::shared_ptr<RemoteVideoTrack> RemoteParticipantImpl::videoTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    if (index < _videoTracks->size()) {
        return _videoTracks->at(index);
    }
    return {};
}

std::shared_ptr<RemoteVideoTrack> RemoteParticipantImpl::videoTrack(const std::string& trackSid) const
{
    if (!trackSid.empty()) {
        LOCK_READ_SAFE_OBJ(_videoTracks);
        if (const auto ndx = findBySid(trackSid, _videoTracks.constRef())) {
            return _videoTracks->at(ndx.value());
        }
    }
    return {};
}

bool RemoteParticipantImpl::updateAudio(const TrackInfo& trackInfo) const
{
    if (TrackType::Audio == trackInfo._type && !trackInfo._sid.empty()) {
        LOCK_READ_SAFE_OBJ(_audioTracks);
        if (const auto ndx = findBySid(trackInfo._sid, _audioTracks.constRef())) {
            _audioTracks->at(ndx.value())->setInfo(trackInfo);
            return true;
        }
    }
    return false;
}

bool RemoteParticipantImpl::updateVideo(const TrackInfo& trackInfo) const
{
    if (TrackType::Video == trackInfo._type && !trackInfo._sid.empty()) {
        LOCK_READ_SAFE_OBJ(_videoTracks);
        if (const auto ndx = findBySid(trackInfo._sid, _videoTracks.constRef())) {
            _videoTracks->at(ndx.value())->setInfo(trackInfo);
            return true;
        }
    }
    return false;
}

const TrackInfo* RemoteParticipantImpl::findBySid(const std::string& trackSid) const
{
    if (!trackSid.empty()) {
        for (const auto& trackInfo : _info->_tracks) {
            if (trackInfo._sid == trackSid) {
                return &trackInfo;
            }
        }
    }
    return nullptr;
}

TrackInfo* RemoteParticipantImpl::findBySid(const std::string& trackSid)
{
    if (!trackSid.empty()) {
        for (auto& trackInfo : _info->_tracks) {
            if (trackInfo._sid == trackSid) {
                return &trackInfo;
            }
        }
    }
    return nullptr;
}

template <class TTrack>
bool RemoteParticipantImpl::addTrack(const std::string& trackSid,
                                     const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                     Bricks::SafeObj<Tracks<TTrack>>& collection) const
{
    bool added = false;
    if (!trackSid.empty()) {
        if (auto device = makeDevice<TTrack>(receiver)) {
            LOCK_READ_SAFE_OBJ(_info);
            if (const auto trackInfo = findBySid(trackSid)) {
                auto trackImpl = std::make_shared<TTrack>(*trackInfo,
                                                          std::move(receiver),
                                                          std::move(device),
                                                          _securityFactory);
                if (attachCryptor(trackInfo->_encryption, receiver)) {
                    {
                        const std::lock_guard guard(collection.mutex());
                        collection->push_back(std::move(trackImpl));
                    }
                    _listener->invoke(&RemoteParticipantListener::onRemoteTrackAdded,
                                      trackInfo->_type, trackInfo->_encryption, trackSid);
                    added = true;
                }
                else {
                    _listener->invoke(&RemoteParticipantListener::onTrackCryptoError,
                                      trackInfo->_type, trackInfo->_encryption,
                                      trackSid, E2ECryptoError::CryptorCreationFailure);
                }
            }
        }
    }
    return added;
}

template <class TTrack>
bool RemoteParticipantImpl::removeTrack(const std::string& trackSid,
                                        Bricks::SafeObj<Tracks<TTrack>>& collection) const
{
    if (!trackSid.empty()) {
        std::shared_ptr<TTrack> removed;
        {
            const std::lock_guard guard(collection.mutex());
            if (const auto ndx = findBySid(trackSid, collection.constRef())) {
                removed = collection->at(ndx.value());
                collection->erase(collection->begin() + ndx.value());
            }
        }
        if (removed) {
            _listener->invoke(&RemoteParticipantListener::onRemoteTrackRemoved,
                              removed->type(), removed->sid());
            return true;
        }
    }
    return false;
}

template <class TTrack>
void RemoteParticipantImpl::clearTracks(Bricks::SafeObj<Tracks<TTrack>>& collection) const
{
    Tracks<TTrack> removed;
    {
        const std::lock_guard guard(collection.mutex());
        removed = collection.take();
    }
    for (const auto& track : removed) {
        _listener->invoke(&RemoteParticipantListener::onRemoteTrackRemoved,
                          track->type(), track->sid());
    }
}

bool RemoteParticipantImpl::attachCryptor(EncryptionType encryption,
                                          const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) const
{
    if (receiver) {
        switch (encryption) {
            case EncryptionType::None:
                return true;
            case EncryptionType::Gcm:
                if (_securityFactory && receiver) {
                    auto cryptor = _securityFactory->createCryptor(false,
                                                                   receiver->media_type(),
                                                                   identity(),
                                                                   receiver->id());
                    if (cryptor) {
                        cryptor->setObserver(_listener);
                        receiver->SetFrameTransformer(std::move(cryptor));
                        return true;
                    }
                }
                break;
            default:
                // TODO: log error
                break;
        }
    }
    return false;
}

template <class TTrack>
std::optional<size_t> RemoteParticipantImpl::findBySid(const std::string& trackSid,
                                                       const Tracks<TTrack>& collection)
{
    if (!trackSid.empty()) {
        if (const auto s = collection.size()) {
            for (size_t i = 0U; i < s; ++i) {
                if (trackSid == collection[i]->sid()) {
                    return i;
                }
            }
        }
    }
    return std::nullopt;
}

RemoteParticipantImpl::ListenerImpl::ListenerImpl(RemoteParticipantImpl* owner)
    : _owner(owner)
{
}

template <class Method, typename... Args>
void RemoteParticipantImpl::ListenerImpl::invoke(const Method& method, Args&&... args) const
{
    LOCK_READ_SAFE_OBJ(_owner);
    if (const auto owner = _owner.constRef()) {
        _listener.invoke(method, owner, std::forward<Args>(args)...);
    }
}

void RemoteParticipantImpl::ListenerImpl::reset()
{
    _owner(nullptr);
    _listener.reset();
}

void RemoteParticipantImpl::ListenerImpl::
    onDecryptionStateChanged(cricket::MediaType mediaType, const std::string&,
                             const std::string& trackSid, AesCgmCryptorState state)
{
    if (const auto err = toCryptoError(state)) {
        invoke(&ParticipantListener::onTrackCryptoError,
               mediaTypeToTrackType(mediaType),
               EncryptionType::Gcm, trackSid, err.value());
    }
}

} // namespace LiveKitCpp
