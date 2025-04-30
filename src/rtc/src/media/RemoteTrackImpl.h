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
#pragma once // RemoteTrackImpl.h
#include "Logger.h"
#include "TrackManager.h"
#include "SafeObj.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include "livekit/rtc/media/NetworkPriority.h"
#include "livekit/signaling/sfu/TrackInfo.h"
#include <api/scoped_refptr.h>
#include <api/rtp_receiver_interface.h>
#include <type_traits>

namespace LiveKitCpp
{

class Track;

template <class TBaseImpl>
class RemoteTrackImpl : public TBaseImpl
{
    static_assert(std::is_base_of_v<Track, TBaseImpl>);
public:
    void setInfo(const TrackInfo& info);
    cricket::MediaType mediaType() const;
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    std::string sid() const final;
    EncryptionType encryption() const final;
    BackupCodecPolicy backupCodecPolicy() const final;
    std::string id() const final;
    std::string name() const final;
    bool remote() const noexcept final { return true; }
    TrackSource source() const final;
    bool remoteMuted() const final;
protected:
    template <class TMediaDevice>
    RemoteTrackImpl(const TrackInfo& initialInfo,
                    const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                    std::shared_ptr<TMediaDevice> mediaDevice,
                    const std::weak_ptr<TrackManager>& trackManager);
    const auto& info() const noexcept { return _info; }
    void onMuteChanged(bool mute) const final;
private:
    Bricks::SafeObj<TrackInfo> _info;
    const rtc::scoped_refptr<webrtc::RtpReceiverInterface> _receiver;
};

template <class TBaseImpl>
template <class TMediaDevice>
inline RemoteTrackImpl<TBaseImpl>::RemoteTrackImpl(const TrackInfo& initialInfo,
                                                   const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                                   std::shared_ptr<TMediaDevice> mediaDevice,
                                                   const std::weak_ptr<TrackManager>& trackManager)
    : TBaseImpl(std::move(mediaDevice), trackManager)
    , _info(initialInfo)
    , _receiver(receiver)
{
}

template <class TBaseImpl>
inline void RemoteTrackImpl<TBaseImpl>::setInfo(const TrackInfo& info)
{
    bool remoteMuteChanged = false, nameChanged = false;
    {
        LOCK_WRITE_SAFE_OBJ(_info);
        remoteMuteChanged = _info->_muted != info._muted;
        nameChanged = _info->_name != info._name;
        _info = info;
    }
    if (remoteMuteChanged) {
        TBaseImpl::notify(&MediaEventsListener::onRemoteSideMuteChanged, id(), info._muted);
    }
    if (nameChanged) {
        TBaseImpl::notify(&MediaEventsListener::onNameChanged, id(), info._name);
    }
}

template <class TBaseImpl>
inline cricket::MediaType RemoteTrackImpl<TBaseImpl>::mediaType() const
{
    if (_receiver) {
        return _receiver->media_type();
    }
    return cricket::MEDIA_TYPE_UNSUPPORTED;
}

template <class TBaseImpl>
inline void RemoteTrackImpl<TBaseImpl>::queryStats() const
{
    if (const auto m = TBaseImpl::trackManager()) {
        m->queryStats(_receiver, TBaseImpl::statsCollector());
    }
}

template <class TBaseImpl>
inline std::string RemoteTrackImpl<TBaseImpl>::sid() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_sid;
}

template <class TBaseImpl>
inline EncryptionType RemoteTrackImpl<TBaseImpl>::encryption() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_encryption;
}

template <class TBaseImpl>
inline BackupCodecPolicy RemoteTrackImpl<TBaseImpl>::backupCodecPolicy() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_backupCodecPolicy;
}

template <class TBaseImpl>
inline std::string RemoteTrackImpl<TBaseImpl>::id() const
{
    return _receiver ? _receiver->id() : std::string{};
}

template <class TBaseImpl>
inline std::string RemoteTrackImpl<TBaseImpl>::name() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_name;
}

template <class TBaseImpl>
inline TrackSource RemoteTrackImpl<TBaseImpl>::source() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_source;
}

template <class TBaseImpl>
inline bool RemoteTrackImpl<TBaseImpl>::remoteMuted() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_muted;
}

template <class TBaseImpl>
inline void RemoteTrackImpl<TBaseImpl>::onMuteChanged(bool mute) const
{
    TBaseImpl::onMuteChanged(mute);
    if (const auto m = TBaseImpl::trackManager()) {
        m->notifyAboutMuteChanges(sid(), mute);
    }
}

} // namespace LiveKitCpp
