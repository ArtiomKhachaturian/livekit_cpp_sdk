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
#pragma once // LocalVideoTrack.h
#include "LocalTrackImpl.h"
#include "VideoTrackImpl.h"
#include "VideoTrack.h"

namespace LiveKitCpp
{

template<class TRtcTrack = webrtc::VideoTrackInterface, class TTrackApi = VideoTrack>
class LocalVideoTrackImpl : public LocalTrackImpl<TRtcTrack, VideoTrackImpl<TTrackApi>>
{
    using Base = LocalTrackImpl<TRtcTrack, VideoTrackImpl<TTrackApi>>;
protected:
    LocalVideoTrackImpl(std::string name, LocalTrackManager* manager,
                        const std::shared_ptr<Bricks::Logger>& logger = {});
    static void installSink(bool install,
                            rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                            const webrtc::scoped_refptr<webrtc::VideoTrackInterface>& track);
    // impl. of VideoTrackImpl<>
    void installSink(bool install, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) final;
};

template<class TRtcTrack, class TTrackApi>
inline LocalVideoTrackImpl<TRtcTrack, TTrackApi>::
    LocalVideoTrackImpl(std::string name, LocalTrackManager* manager,
                        const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(name), manager, logger)
{
}

template<class TRtcTrack, class TTrackApi>
inline void LocalVideoTrackImpl<TRtcTrack, TTrackApi>::
    installSink(bool install, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                const webrtc::scoped_refptr<webrtc::VideoTrackInterface>& track)
{
    if (sink && track) {
        if (install) {
            track->AddOrUpdateSink(sink, {});
        }
        else {
            track->RemoveSink(sink);
        }
    }
}

template<class TRtcTrack, class TTrackApi>
inline void LocalVideoTrackImpl<TRtcTrack, TTrackApi>::
    installSink(bool install, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    installSink(install, sink, Base::mediaTrack());
}

} // namespace LiveKitCpp
