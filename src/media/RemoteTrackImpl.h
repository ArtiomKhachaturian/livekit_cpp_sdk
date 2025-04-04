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
#ifdef WEBRTC_AVAILABLE
#include "Logger.h"
#include "TrackManager.h"
#include "rtc/TrackInfo.h"
#include <api/scoped_refptr.h>
#include <api/rtp_receiver_interface.h>
#include <type_traits>

namespace LiveKitCpp
{

class Track;

template<class TBaseImpl>
class RemoteTrackImpl : public TBaseImpl
{
    static_assert(std::is_base_of_v<Track, TBaseImpl>);
public:
    // impl. of StatsSource
    void queryStats() const final;
    // impl. of Track
    EncryptionType encryption() const final { return _info._encryption; }
    BackupCodecPolicy backupCodecPolicy() const final { return _info._backupCodecPolicy; }
    std::string id() const final { return _info._sid; }
    std::string name() const final { return _info._name; }
    bool remote() const noexcept final { return true; }
    TrackSource source() const final { return _info._source; }
protected:
    template<class TWebRtcTrack>
    RemoteTrackImpl(const TrackInfo& info,
                    const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                    webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                    TrackManager* manager);
    const auto& info() const noexcept { return _info; }
    void notifyAboutMuted(bool mute) const override;
private:
    const TrackInfo _info;
    const rtc::scoped_refptr<webrtc::RtpReceiverInterface> _receiver;
};

template<class TBaseImpl>
template<class TWebRtcTrack>
inline RemoteTrackImpl<TBaseImpl>::RemoteTrackImpl(const TrackInfo& info,
                                                   const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                                   webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                                                   TrackManager* manager)
    : TBaseImpl(std::move(mediaTrack), manager)
    , _info(info)
    , _receiver(receiver)
{
}

template<class TBaseImpl>
inline void RemoteTrackImpl<TBaseImpl>::queryStats() const
{
    if (const auto m = TBaseImpl::manager()) {
        m->queryStats(_receiver, TBaseImpl::statsCollector());
    }
}

template<class TBaseImpl>
inline void RemoteTrackImpl<TBaseImpl>::notifyAboutMuted(bool mute) const
{
    TBaseImpl::notifyAboutMuted(mute);
    if (const auto m = TBaseImpl::manager()) {
        m->notifyAboutMuteChanges(_info._sid, mute);
    }
}

} // namespace LiveKitCpp
#endif
