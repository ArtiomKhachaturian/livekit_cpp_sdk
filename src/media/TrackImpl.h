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
#pragma once // TrackImpl.h
#include "StatsSourceImpl.h"
#include "TrackManager.h"
#include <api/media_stream_interface.h>
#include <atomic>

namespace LiveKitCpp
{

template<class TWebRtcTrack, class TTrackApi>
class TrackImpl : public TTrackApi
{
    static_assert(std::is_base_of_v<Track, TTrackApi>);
    static_assert(std::is_base_of_v<webrtc::MediaStreamTrackInterface, TWebRtcTrack>);
public:
    ~TrackImpl() override { _statsCollector->clearListeners(); }
    // impl. of StatsSource
    void addListener(StatsListener* listener) final;
    void removeListener(StatsListener* listener) final;
    // impl. of Track
    bool live() const final;
    void mute(bool mute) final; // request media track creation if needed
    bool muted() const override { return _muted; }
protected:
    TrackImpl(webrtc::scoped_refptr<TWebRtcTrack> mediaTrack, TrackManager* manager);
    const auto& mediaTrack() const noexcept { return _mediaTrack; }
    TrackManager* manager() const noexcept { return _manager; }
    webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> statsCollector() const;
    virtual void notifyAboutMuted(bool /*mute*/) const {}
private:
    const webrtc::scoped_refptr<StatsSourceImpl> _statsCollector;
    const webrtc::scoped_refptr<TWebRtcTrack> _mediaTrack;
    TrackManager* const _manager;
    std::atomic_bool _muted = false;
};

template<class TWebRtcTrack, class TTrackApi>
inline TrackImpl<TWebRtcTrack, TTrackApi>::TrackImpl(webrtc::scoped_refptr<TWebRtcTrack> mediaTrack,
                                                     TrackManager* manager)
    : _statsCollector(webrtc::make_ref_counted<StatsSourceImpl>())
    , _mediaTrack(std::move(mediaTrack))
    , _manager(manager)
    , _muted(!_mediaTrack || !_mediaTrack->enabled())
{
}

template<class TRtcTrack, class TTrackApi>
inline void TrackImpl<TRtcTrack, TTrackApi>::addListener(StatsListener* listener)
{
    _statsCollector->addListener(listener);
}

template<class TRtcTrack, class TTrackApi>
inline void TrackImpl<TRtcTrack, TTrackApi>::removeListener(StatsListener* listener)
{
    _statsCollector->removeListener(listener);
}

template<class TRtcTrack, class TTrackApi>
inline bool TrackImpl<TRtcTrack, TTrackApi>::live() const
{
    return _mediaTrack && webrtc::MediaStreamTrackInterface::kLive == _mediaTrack->state();
}

template<class TRtcTrack, class TTrackApi>
inline void TrackImpl<TRtcTrack, TTrackApi>::mute(bool mute)
{
    if (_mediaTrack && mute != _muted.exchange(mute)) {
        const auto wasEnabled = _mediaTrack->enabled();
        if (wasEnabled == mute) {
            _mediaTrack->set_enabled(!mute);
            notifyAboutMuted(mute);
        }
    }
}

template<class TRtcTrack, class TTrackApi>
inline webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>
    TrackImpl<TRtcTrack, TTrackApi>::statsCollector() const
{
    return _statsCollector;
}

} // namespace LiveKitCpp
