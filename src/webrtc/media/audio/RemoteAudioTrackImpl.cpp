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
#include "RemoteAudioTrackImpl.h"

namespace LiveKitCpp
{

RemoteAudioTrackImpl::RemoteAudioTrackImpl(TrackManager* manager, const TrackInfo& info,
                                           const webrtc::scoped_refptr<webrtc::AudioTrackInterface>& track)
    : Base(manager, info, track)
{
}

RemoteAudioTrackImpl::RemoteAudioTrackImpl(TrackManager* manager, const TrackInfo& info,
                                           webrtc::AudioTrackInterface* track)
    : RemoteAudioTrackImpl(manager, info, webrtc::scoped_refptr<webrtc::AudioTrackInterface>(track))
{
}

RemoteAudioTrackImpl::~RemoteAudioTrackImpl()
{
    installSink(false, audioSink());
}

void RemoteAudioTrackImpl::installSink(bool install, webrtc::AudioTrackSinkInterface* sink)
{
    if (sink && track()) {
        if (install) {
            track()->AddSink(sink);
        }
        else {
            track()->RemoveSink(sink);
        }
    }
}

bool RemoteAudioTrackImpl::signalLevel(int& level) const
{
    return track() && track()->GetSignalLevel(&level);
}

} // namespace LiveKitCpp
