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
#include "TrackManager.h"
#include "SafeObj.h"
#include "rtc/TrackInfo.h"
#include <api/media_stream_interface.h>
#include <api/scoped_refptr.h>
#include <type_traits>

namespace LiveKitCpp
{

class Track;

template<class TRtcTrack, class TTrackApi>
class RemoteTrackImpl : public TTrackApi
{
    static_assert(std::is_base_of_v<Track, TTrackApi>);
public:
    // impl. of Track
    bool remote() const noexcept final { return true; }
    bool live() const final;
    TrackSource source() const final { return _info._source; }
    std::string sid() const final { return _info._sid; }
    void mute(bool mute) final;
    bool muted() const final;
protected:
    RemoteTrackImpl(TrackManager* manager, const TrackInfo& info,
                    const webrtc::scoped_refptr<TRtcTrack>& track);
    const auto& info() const noexcept { return _info; }
    const auto& track() const noexcept { return _track; }
private:
    TrackManager* const _manager;
    const TrackInfo _info;
    const webrtc::scoped_refptr<TRtcTrack> _track;
};

template<class TRtcTrack, class TTrackApi>
inline RemoteTrackImpl<TRtcTrack, TTrackApi>::RemoteTrackImpl(TrackManager* manager,
                                                              const TrackInfo& info,
                                                              const webrtc::scoped_refptr<TRtcTrack>& track)
    : _manager(manager)
    , _info(info)
    , _track(track)
{
    if (_manager && _track && info._muted != muted()) {
        _manager->notifyAboutMuteChanges(sid(), muted());
    }
}

template<class TRtcTrack, class TTrackApi>
inline bool RemoteTrackImpl<TRtcTrack, TTrackApi>::live() const
{
    if (_track) {
        return webrtc::MediaStreamTrackInterface::kLive == _track->state();
    }
    return false;
}

template<class TRtcTrack, class TTrackApi>
inline void RemoteTrackImpl<TRtcTrack, TTrackApi>::mute(bool mute)
{
    if (_track && mute == _track->enabled() && _track->set_enabled(!mute)
        && _manager) {
        _manager->notifyAboutMuteChanges(sid(), mute);
    }
}

template<class TRtcTrack, class TTrackApi>
inline bool RemoteTrackImpl<TRtcTrack, TTrackApi>::muted() const
{
    return !_track || !_track->enabled();
}

} // namespace LiveKitCpp
