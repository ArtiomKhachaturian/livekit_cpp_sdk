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
#ifdef WEBRTC_AVAILABLE
#include "Logger.h"
#include "LocalTrack.h"
#include "SafeScopedRefPtr.h"
#include "rtc/AddTrackRequest.h"
#include "Track.h"
#include "TrackManager.h"
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template<class TBaseImpl>
class LocalTrackImpl : public TBaseImpl, public LocalTrack
{
    static_assert(std::is_base_of_v<Track, TBaseImpl>);
public:
    ~LocalTrackImpl() override = default;
    // impl. of LocalTrack
    void close() override;
    void setSid(const std::string& sid) final { _sid(sid); }
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> media() const final;
    void notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender,
                                         bool encryption) final;
    void notifyThatMediaRemovedFromTransport() final;
    bool fillRequest(AddTrackRequest* request) const override;
    bool muted() const final { return TBaseImpl::muted(); }
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    std::string sid() const final { return _sid(); }
    EncryptionType encryption() const final;
    std::string id() const final { return cid(); }
    std::string name() const final { return _name; }
    bool remote() const noexcept final { return false; }
protected:
    template<class TWebRtcTrack>
    LocalTrackImpl(std::string name,
                   webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                   TrackManager* manager);
    void notifyAboutMuted(bool mute) const override;
private:
    bool added() const;
private:
    const std::string _name;
    std::atomic_bool _encryption = false;
    Bricks::SafeObj<std::string> _sid;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
};

template<class TBaseImpl>
template<class TWebRtcTrack>
inline LocalTrackImpl<TBaseImpl>::LocalTrackImpl(std::string name,
                                                 webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                                                 TrackManager* manager)
    : TBaseImpl(std::move(mediaTrack), manager)
    , _name(std::move(name))
{
}

template<class TBaseImpl>
void LocalTrackImpl<TBaseImpl>::close()
{
    LocalTrack::close();
    notifyThatMediaRemovedFromTransport();
}

template<class TBaseImpl>
inline webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalTrackImpl<TBaseImpl>::media() const
{
    return TBaseImpl::mediaTrack();
}

template<class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::
    notifyThatMediaAddedToTransport(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender,
                                    bool encryption)
{
    if (sender && sender->track() == TBaseImpl::mediaTrack()) {
        _sender(std::move(sender));
        _encryption = encryption;
        notifyAboutMuted(muted());
    }
}

template<class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyThatMediaRemovedFromTransport()
{
    _sender({});
    _encryption = false;
}

template<class TBaseImpl>
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

template<class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::queryStats() const
{
    if (const auto m = TBaseImpl::manager()) {
        m->queryStats(_sender(), TBaseImpl::statsCollector());
    }
}

template<class TBaseImpl>
inline EncryptionType LocalTrackImpl<TBaseImpl>::encryption() const
{
    if (TBaseImpl::manager() && _encryption) {
        return TBaseImpl::manager()->localEncryptionType();
    }
    return EncryptionType::None;
}

template<class TBaseImpl>
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

template<class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::added() const
{
    LOCK_READ_SAFE_OBJ(_sender);
    return nullptr != _sender->get();
}

} // namespace LiveKitCpp
#endif
