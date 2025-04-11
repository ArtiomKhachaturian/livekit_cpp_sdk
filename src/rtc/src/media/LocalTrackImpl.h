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
#include "LocalTrack.h"
#include "SafeScopedRefPtr.h"
#include "TrackManager.h"
#include "Utils.h"
#include "livekit/signaling/sfu/AddTrackRequest.h"
#include "livekit/rtc/media/Track.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template <class TBaseImpl>
class LocalTrackImpl : public TBaseImpl, public LocalTrack
{
    static_assert(std::is_base_of_v<Track, TBaseImpl>);
public:
    ~LocalTrackImpl() override = default;
    // impl. of LocalTrack
    void setRemoteSideMute(bool mute) override;
    void close() override;
    void setSid(const std::string& sid) final;
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> media() const final;
    void notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender,
                                         bool encryption) final;
    void notifyThatMediaRemovedFromTransport() final;
    bool fillRequest(AddTrackRequest* request) const override;
    bool remoteMuted() const final { return _remoteMuted; }
    bool muted() const override { return TBaseImpl::muted(); }
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    std::string sid() const final { return _sid(); }
    EncryptionType encryption() const final;
    std::string name() const final { return _name; }
    bool remote() const noexcept final { return false; }
protected:
    template <class TMediaDevice>
    LocalTrackImpl(std::string name,
                   std::shared_ptr<TMediaDevice> mediaDevice,
                   TrackManager* manager);
    // overrides of TrackImpl<>
    void notifyAboutMuted(bool mute) const final;
private:
    bool added() const;
private:
    const std::string _name;
    std::atomic_bool _encryption = false;
    std::atomic_bool _remoteMuted = false;
    Bricks::SafeObj<std::string> _sid;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
};

template <class TBaseImpl>
template <class TMediaDevice>
inline LocalTrackImpl<TBaseImpl>::LocalTrackImpl(std::string name,
                                                 std::shared_ptr<TMediaDevice> mediaDevice,
                                                 TrackManager* manager)
    : TBaseImpl(std::move(mediaDevice), manager)
    , _name(std::move(name))
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
    LocalTrack::close();
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
inline void LocalTrackImpl<TBaseImpl>::
    notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender,
                                    bool encryption)
{
    if (sender && sender->track() == media()) {
        _sender(std::move(sender));
        _encryption = encryption;
        notifyAboutMuted(muted());
    }
}

template <class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyThatMediaRemovedFromTransport()
{
    _sender({});
    _encryption = false;
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
    if (const auto m = TBaseImpl::manager()) {
        m->queryStats(_sender(), TBaseImpl::statsCollector());
    }
}

template <class TBaseImpl>
inline EncryptionType LocalTrackImpl<TBaseImpl>::encryption() const
{
    if (TBaseImpl::manager() && _encryption) {
        return TBaseImpl::manager()->localEncryptionType();
    }
    return EncryptionType::None;
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
    if (TBaseImpl::manager() && added()) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            TBaseImpl::manager()->notifyAboutMuteChanges(sid, mute);
        }
    }
}

} // namespace LiveKitCpp
