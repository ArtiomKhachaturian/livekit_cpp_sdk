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
#include "TrackImpl.h"
#include "VideoDeviceImpl.h"
#include "livekit/rtc/media/VideoTrack.h"
#include <type_traits>

namespace LiveKitCpp
{

template <class TMediaDevice = VideoDeviceImpl, class TTrackApi = VideoTrack>
class VideoTrackImpl : public TrackImpl<TMediaDevice, TTrackApi>
{
    static_assert(std::is_base_of_v<VideoTrack, TTrackApi>);
    static_assert(std::is_base_of_v<VideoDevice, TMediaDevice>);
    using Base = TrackImpl<TMediaDevice, TTrackApi>;
public:
    ~VideoTrackImpl() override = default;
    // impl. of VideoTrack
    void addSink(VideoSink* sink) final;
    void removeSink(VideoSink* sink) final;
protected:
    VideoTrackImpl(std::shared_ptr<TMediaDevice> mediaDevice,
                   const std::weak_ptr<TrackManager>& trackManager);
};

template <class TMediaDevice, class TTrackApi>
inline VideoTrackImpl<TMediaDevice, TTrackApi>::
    VideoTrackImpl(std::shared_ptr<TMediaDevice> mediaDevice, const std::weak_ptr<TrackManager>& trackManager)
        : Base(std::move(mediaDevice), trackManager)
{
}

template <class TMediaDevice, class TTrackApi>
inline void VideoTrackImpl<TMediaDevice, TTrackApi>::addSink(VideoSink* sink)
{
    if (const auto& md = Base::mediaDevice()) {
        md->addSink(sink);
    }
}

template <class TMediaDevice, class TTrackApi>
inline void VideoTrackImpl<TMediaDevice, TTrackApi>::removeSink(VideoSink* sink)
{
    if (const auto& md = Base::mediaDevice()) {
        md->removeSink(sink);
    }
}

} // namespace LiveKitCpp
