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
#pragma once // VideoTrackImpl.h
#ifdef WEBRTC_AVAILABLE
#include "TrackImpl.h"
#include "VideoTrack.h"
#include "VideoSinks.h"
#include <type_traits>

namespace LiveKitCpp
{

template<class TTrackApi = VideoTrack, class TWebRtcTrack = webrtc::VideoTrackInterface>
class VideoTrackImpl : public TrackImpl<TWebRtcTrack, TTrackApi>
{
    static_assert(std::is_base_of_v<VideoTrack, TTrackApi>);
    static_assert(std::is_base_of_v<webrtc::VideoTrackInterface, TWebRtcTrack>);
    using Base = TrackImpl<TWebRtcTrack, TTrackApi>;
public:
    ~VideoTrackImpl() override;
    // impl. of VideoTrack
    void addSink(VideoTrackSink* sink) final;
    void removeSink(VideoTrackSink* sink) final;
protected:
    VideoTrackImpl(webrtc::scoped_refptr<TWebRtcTrack> videoTrack, TrackManager* manager);
private:
    VideoSinks _sinks;
};

template<class TTrackApi, class TWebRtcTrack>
inline VideoTrackImpl<TTrackApi, TWebRtcTrack>::
    VideoTrackImpl(webrtc::scoped_refptr<TWebRtcTrack> videoTrack, TrackManager* manager)
    : Base(std::move(videoTrack), manager)
{
}

template<class TTrackApi, class TWebRtcTrack>
inline VideoTrackImpl<TTrackApi, TWebRtcTrack>::~VideoTrackImpl()
{
    if (_sinks.clear()) {
        if (const auto& t = Base::mediaTrack()) {
            t->RemoveSink(&_sinks);
        }
    }
}

template<class TTrackApi, class TWebRtcTrack>
inline void VideoTrackImpl<TTrackApi, TWebRtcTrack>::addSink(VideoTrackSink* sink)
{
    if (Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        if (const auto& t = Base::mediaTrack()) {
            t->AddOrUpdateSink(&_sinks, {});
        }
    }
}

template<class TTrackApi, class TWebRtcTrack>
inline void VideoTrackImpl<TTrackApi, TWebRtcTrack>::removeSink(VideoTrackSink* sink)
{
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink)) {
        if (const auto& t = Base::mediaTrack()) {
            t->RemoveSink(&_sinks);
        }
    }
}

} // namespace LiveKitCpp
#endif
