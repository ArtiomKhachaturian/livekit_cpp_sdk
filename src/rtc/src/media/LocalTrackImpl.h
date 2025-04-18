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
#pragma once // LocalTrack.h
#include "Logger.h"
#include "LocalTrackAccessor.h"
#include "SafeScopedRefPtr.h"
#include "TrackManager.h"
#include "Utils.h"
#include "livekit/signaling/sfu/AddTrackRequest.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include "livekit/rtc/media/Track.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template <class TBaseImpl>
class LocalTrackImpl : public TBaseImpl, public LocalTrackAccessor
{
    static_assert(std::is_base_of_v<Track, TBaseImpl>);
public:
    ~LocalTrackImpl() override = default;
    // impl. of LocalTrack
    void setRemoteSideMute(bool mute) override;
    void close() override;
    void setSid(const std::string& sid) final;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> media() const final;
    EncryptionType notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) final;
    void notifyThatMediaRemovedFromTransport() final;
    bool fillRequest(AddTrackRequest* request) const override;
    bool remoteMuted() const final { return _remoteMuted; }
    bool muted() const override { return TBaseImpl::muted(); }
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    std::string sid() const final { return _sid(); }
    EncryptionType encryption() const final { return _encryption; }
    std::string name() const final { return _name; }
    bool remote() const noexcept final { return false; }
protected:
    template <class TMediaDevice>
    LocalTrackImpl(std::string name,
                   EncryptionType encryption,
                   std::shared_ptr<TMediaDevice> mediaDevice,
                   const std::weak_ptr<TrackManager>& trackManager);
    // overrides of TrackImpl<>
    void notifyAboutMuted(bool mute) const final;
private:
    bool added() const;
private:
    const std::string _name;
    const EncryptionType _encryption;
    std::atomic_bool _remoteMuted = false;
    Bricks::SafeObj<std::string> _sid;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
};

template <class TBaseImpl>
template <class TMediaDevice>
inline LocalTrackImpl<TBaseImpl>::LocalTrackImpl(std::string name,
                                                 EncryptionType encryption,
                                                 std::shared_ptr<TMediaDevice> mediaDevice,
                                                 const std::weak_ptr<TrackManager>& trackManager)
    : TBaseImpl(std::move(mediaDevice), trackManager)
    , _name(std::move(name))
    , _encryption(encryption)
{
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setRemoteSideMute(bool mute)
{
    if (exchangeVal(mute, _remoteMuted)) {
        TBaseImpl::notify(&MediaEventsListener::onRemoteSideMuteChanged, TBaseImpl::id(), mute);
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::close()
{
    LocalTrackAccessor::close();
    notifyThatMediaRemovedFromTransport();
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::setSid(const std::string& sid)
{
    if (exchangeVal(sid, _sid)) {
        TBaseImpl::notify(&MediaEventsListener::onSidChanged, TBaseImpl::id(), sid);
    }
}

template <class TBaseImpl>
inline webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalTrackImpl<TBaseImpl>::media() const
{
    if (const auto& md = TBaseImpl::mediaDevice()) {
        return md->track();
    }
    return {};
}

template <class TBaseImpl>
inline EncryptionType LocalTrackImpl<TBaseImpl>::
    notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender && sender->track() == media()) {
        _sender(std::move(sender));
        notifyAboutMuted(muted());
        return _encryption;
    }
    return EncryptionType::None;
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyThatMediaRemovedFromTransport()
{
    _sender({});
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::fillRequest(AddTrackRequest* request) const
{
    if (request && media() && added()) {
        request->_encryption = encryption();
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
        return true;
    }
    return false;
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::queryStats() const
{
    if (const auto m = TBaseImpl::trackManager()) {
        m->queryStats(_sender(), TBaseImpl::statsCollector());
    }
}

template <class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::added() const
{
    LOCK_READ_SAFE_OBJ(_sender);
    return nullptr != _sender->get();
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyAboutMuted(bool mute) const
{
    TBaseImpl::notifyAboutMuted(mute);
    if (added()) {
        if (const auto m = TBaseImpl::trackManager()) {
            const auto sid = this->sid();
            if (!sid.empty()) {
                m->notifyAboutMuteChanges(sid, mute);
            }
        }
    }
}

} // namespace LiveKitCpp
