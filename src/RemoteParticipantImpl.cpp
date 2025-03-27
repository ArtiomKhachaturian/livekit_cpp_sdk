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
#include "RemoteParticipantImpl.h"
#include "RemoteAudioTrackImpl.h"
#include "RemoteVideoTrackImpl.h"
#include "E2ESecurityFactory.h"
#include "AesCgmCryptor.h"
#include "e2e/AesCgmCryptorObserver.h"
#include "e2e/E2ECryptoError.h"
#include "Seq.h"
#include <api/media_stream_interface.h>
#include <api/rtp_receiver_interface.h>
#include <optional>

namespace {

using namespace LiveKitCpp;

inline bool compareTrackInfo(const TrackInfo& l, const TrackInfo& r) {
    return l._sid == r._sid;
}

template<class TMediaInterace>
inline webrtc::scoped_refptr<TMediaInterace> mediaTrack(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) {
    if (receiver) {
        if (const auto media = dynamic_cast<TMediaInterace*>(receiver->track().get())) {
            return webrtc::scoped_refptr<TMediaInterace>(media);
        }
    }
    return {};
}

template <class TTrack>
struct MediaInteraceType {};

template <>
struct MediaInteraceType<RemoteAudioTrackImpl>
{
    using MediaInterace = webrtc::AudioTrackInterface;
};

template <>
struct MediaInteraceType<RemoteVideoTrackImpl>
{
    using MediaInterace = webrtc::VideoTrackInterface;
};

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
                                             const ParticipantInfo& info)
    : _securityFactory(securityFactory)
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

std::optional<TrackType> RemoteParticipantImpl::trackType(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(sid)) {
            return trackInfo->_type;
        }
    }
    return std::nullopt;
}

bool RemoteParticipantImpl::addAudio(const std::string& sid,
                                     const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    return addTrack<RemoteAudioTrackImpl>(sid, receiver, _audioTracks);
}

bool RemoteParticipantImpl::addVideo(const std::string& sid,
                                     const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    return addTrack<RemoteVideoTrackImpl>(sid, receiver, _videoTracks);
}

bool RemoteParticipantImpl::removeAudio(const std::string& sid)
{
    return removeTrack(sid, _audioTracks);
}

bool RemoteParticipantImpl::removeVideo(const std::string& sid)
{
    return removeTrack(sid, _videoTracks);
}

void RemoteParticipantImpl::setInfo(const ParticipantInfo& info)
{
    const auto removed = Seq<TrackInfo>::difference(_info()._tracks,
                                                    info._tracks,
                                                    compareTrackInfo);
    _info(info);
    for (const auto& trackInfo : removed) {
        switch (trackInfo._type) {
            case TrackType::Audio:
                removeAudio(trackInfo._sid);
                break;
            case TrackType::Video:
                removeVideo(trackInfo._sid);
                break;
            default:
                break;
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

std::shared_ptr<RemoteAudioTrack> RemoteParticipantImpl::audioTrack(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_audioTracks);
        if (const auto ndx = findBySid(sid, _audioTracks.constRef())) {
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

std::shared_ptr<RemoteVideoTrack> RemoteParticipantImpl::videoTrack(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_videoTracks);
        if (const auto ndx = findBySid(sid, _videoTracks.constRef())) {
            return _videoTracks->at(ndx.value());
        }
    }
    return {};
}

const TrackInfo* RemoteParticipantImpl::findBySid(const std::string& sid) const
{
    if (!sid.empty()) {
        for (const auto& trackInfo : _info->_tracks) {
            if (trackInfo._sid == sid) {
                return &trackInfo;
            }
        }
    }
    return nullptr;
}

template<class TTrack>
bool RemoteParticipantImpl::addTrack(const std::string& sid,
                                     const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                     Bricks::SafeObj<Tracks<TTrack>>& collection) const
{
    bool added = false;
    if (!sid.empty()) {
        using MediaInterace = typename MediaInteraceType<TTrack>::MediaInterace;
        if (const auto rtcTrack = mediaTrack<MediaInterace>(receiver)) {
            LOCK_READ_SAFE_OBJ(_info);
            if (const auto trackInfo = findBySid(sid)) {
                auto trackImpl = std::make_shared<TTrack>(_securityFactory,
                                                          *trackInfo,
                                                          rtcTrack);
                if (attachCryptor(trackInfo->_encryption, receiver)) {
                    {
                        const std::lock_guard guard(collection.mutex());
                        collection->push_back(std::move(trackImpl));
                    }
                    _listener->invoke(&RemoteParticipantListener::onRemoteTrackAdded,
                                      trackInfo->_type, trackInfo->_encryption, sid);
                    added = true;
                }
                else {
                    _listener->invoke(&RemoteParticipantListener::onTrackCryptoError,
                                      trackInfo->_type, trackInfo->_encryption,
                                      sid, E2ECryptoError::CryptorCreationFailure);
                }
            }
        }
    }
    return added;
}

template<class TTrack>
bool RemoteParticipantImpl::removeTrack(const std::string& sid,
                                        Bricks::SafeObj<Tracks<TTrack>>& collection) const
{
    if (!sid.empty()) {
        std::shared_ptr<TTrack> removed;
        {
            const std::lock_guard guard(collection.mutex());
            if (const auto ndx = findBySid(sid, collection.constRef())) {
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

template<class TTrack>
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

template<class TTrack>
std::optional<size_t> RemoteParticipantImpl::findBySid(const std::string& sid,
                                                       const Tracks<TTrack>& collection)
{
    if (!sid.empty()) {
        if (const auto s = collection.size()) {
            for (size_t i = 0U; i < s; ++i) {
                if (sid == collection[i]->sid()) {
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
        TrackType type = TrackType::Audio;
        if (cricket::MEDIA_TYPE_VIDEO == mediaType) {
            type = TrackType::Video;
        }
        invoke(&ParticipantListener::onTrackCryptoError, type,
               EncryptionType::Gcm, trackSid, err.value());
    }
}

} // namespace LiveKitCpp
#endif
