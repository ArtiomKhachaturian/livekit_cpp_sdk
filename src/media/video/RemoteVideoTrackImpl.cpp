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
#include "RemoteVideoTrackImpl.h"

namespace LiveKitCpp
{

RemoteVideoTrackImpl::RemoteVideoTrackImpl(TrackManager* manager, const TrackInfo& info,
                                           const webrtc::scoped_refptr<webrtc::VideoTrackInterface>& track)
    : Base(manager, info, track)
{
}

RemoteVideoTrackImpl::RemoteVideoTrackImpl(TrackManager* manager, const TrackInfo& info,
                                           webrtc::VideoTrackInterface* track)
    : RemoteVideoTrackImpl(manager, info, webrtc::scoped_refptr<webrtc::VideoTrackInterface>(track))
{
}

RemoteVideoTrackImpl::~RemoteVideoTrackImpl()
{
    installSink(false, videoSink());
}

void RemoteVideoTrackImpl::installSink(bool install, rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink && track()) {
        if (install) {
            track()->AddOrUpdateSink(sink, {});
        }
        else {
            track()->RemoveSink(sink);
        }
    }
}

} // namespace LiveKitCpp
