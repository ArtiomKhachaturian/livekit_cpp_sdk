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
#include "SafeObj.h"
#include "rtc/AddTrackRequest.h"
#include "Track.h"
#include "TrackManager.h"
#include <api/media_stream_interface.h>
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
    void notifyThatMediaAddedToTransport(bool encryption) final;
    void notifyThatMediaRemovedFromTransport() final;
    bool fillRequest(AddTrackRequest* request) const override;
    bool muted() const final { return TBaseImpl::muted(); }
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
                   TrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger);
    void notifyAboutMuted(bool mute) const override;
private:
    const std::string _name;
    std::atomic_bool _added = false;
    std::atomic_bool _encryption = false;
    Bricks::SafeObj<std::string> _sid;
};

template<class TBaseImpl>
template<class TWebRtcTrack>
inline LocalTrackImpl<TBaseImpl>::LocalTrackImpl(std::string name,
                                                 webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                                                 TrackManager* manager,
                                                 const std::shared_ptr<Bricks::Logger>& logger)
    : TBaseImpl(std::move(mediaTrack), manager, logger)
    , _name(std::move(name))
{
}

template<class TBaseImpl>
void LocalTrackImpl<TBaseImpl>::close()
{
    LocalTrack::close();
    _added = _encryption = false;
}

template<class TBaseImpl>
inline webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalTrackImpl<TBaseImpl>::media() const
{
    return TBaseImpl::mediaTrack();
}

template<class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyThatMediaAddedToTransport(bool encryption)
{
    if (!_added.exchange(true)) {
        _encryption = encryption;
        notifyAboutMuted(muted());
    }
}

template<class TBaseImpl>
inline void LocalTrackImpl<TBaseImpl>::notifyThatMediaRemovedFromTransport()
{
    if (_added.exchange(false)) {
        _encryption = false;
    }
}

template<class TBaseImpl>
inline bool LocalTrackImpl<TBaseImpl>::fillRequest(AddTrackRequest* request) const
{
    if (request && media() && _added) {
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
    if (TBaseImpl::manager() && _added) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            TBaseImpl::manager()->notifyAboutMuteChanges(sid, mute);
        }
    }
}

} // namespace LiveKitCpp
