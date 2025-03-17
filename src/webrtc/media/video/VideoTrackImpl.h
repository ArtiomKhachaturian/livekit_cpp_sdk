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
#include "VideoTrack.h"
#include "VideoTrackSink.h"
#include "Listeners.h"
#include <api/media_stream_interface.h>
#include <api/video/video_sink_interface.h>
#include <api/video/video_frame.h>
#include <type_traits>

namespace LiveKitCpp
{

template<class TTrackApi = VideoTrack>
class VideoTrackImpl : public TTrackApi,
                       protected rtc::VideoSinkInterface<webrtc::VideoFrame>
{
    static_assert(std::is_base_of_v<VideoTrack, TTrackApi>);
public:
    ~VideoTrackImpl() override { _sinks.clear(); }
    // impl. of VideoTrack
    void addSink(VideoTrackSink* sink) final;
    void removeSink(VideoTrackSink* sink) final;
protected:
    VideoTrackImpl() = default;
    bool hasSinks() const noexcept { return !_sinks.empty(); }
    virtual rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack() const = 0;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
private:
    void subscribeVideoTrackSink(bool subscribe);
private:
    Bricks::Listeners<VideoTrackSink*> _sinks;
};

template<class TTrackApi>
inline void VideoTrackImpl<TTrackApi>::addSink(VideoTrackSink* sink)
{
    if (sink) {
        const auto res = _sinks.add(sink);
        if (Bricks::AddResult::OkFirst == res) {
            subscribeVideoTrackSink(true);
        }
    }
}

template<class TTrackApi>
inline void VideoTrackImpl<TTrackApi>::removeSink(VideoTrackSink* sink)
{
    if (sink) {
        const auto res = _sinks.remove(sink);
        if (Bricks::RemoveResult::OkLast == res) {
            subscribeVideoTrackSink(false);
        }
    }
}

template<class TTrackApi>
inline void VideoTrackImpl<TTrackApi>::OnFrame(const webrtc::VideoFrame& /*frame*/)
{
}

template<class TTrackApi>
inline void VideoTrackImpl<TTrackApi>::subscribeVideoTrackSink(bool subscribe)
{
    if (const auto track = videoTrack()) {
        if (subscribe) {
            track->AddOrUpdateSink(this, {});
        }
        else {
            track->RemoveSink(this);
        }
    }
}

} // namespace LiveKitCpp
