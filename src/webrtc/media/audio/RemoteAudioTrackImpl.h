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
#pragma once // RemoteAudioTrackImpl.h
#include "RemoteTrackImpl.h"
#include "AudioTrackImpl.h"
#include "RemoteAudioTrack.h"

namespace LiveKitCpp
{

class RemoteAudioTrackImpl : public RemoteTrackImpl<webrtc::AudioTrackInterface,
                                                    AudioTrackImpl<RemoteAudioTrack>>
{
    using Base = RemoteTrackImpl<webrtc::AudioTrackInterface, AudioTrackImpl<RemoteAudioTrack>>;
public:
    RemoteAudioTrackImpl(TrackManager* manager, const TrackInfo& info,
                         const webrtc::scoped_refptr<webrtc::AudioTrackInterface>& track);
    RemoteAudioTrackImpl(TrackManager* manager, const TrackInfo& info,
                         webrtc::AudioTrackInterface* track);
    ~RemoteAudioTrackImpl() final;
    // impl. of RemoteAudioTrack
    std::string sid() const final { return info()._sid; }
    bool dtx() const final { return !info()._disableDtx; }
    bool stereo() const final { return info()._stereo; }
    bool red() const final { return !info()._disableRed; }
    std::string mime() const final { return info()._mimeType; }
    std::string stream() const final { return info()._stream; }
protected:
    // impl. of AudioTrackImpl
    void installSink(bool install, webrtc::AudioTrackSinkInterface* sink) final;
    bool signalLevel(int& level) const final;
};

} // namespace LiveKitCpp
