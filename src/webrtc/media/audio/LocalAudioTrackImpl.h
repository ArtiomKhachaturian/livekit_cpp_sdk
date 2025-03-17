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
#pragma once // LocalAudioTrack.h
#include "LocalTrackImpl.h"
#include "AudioTrackImpl.h"

namespace LiveKitCpp
{
class LocalAudioTrackImpl : public LocalTrackImpl<webrtc::AudioTrackInterface, AudioTrackImpl<>>
{
    using Base = LocalTrackImpl<webrtc::AudioTrackInterface, AudioTrackImpl<>>;
public:
    LocalAudioTrackImpl(LocalTrackManager* manager, bool microphone = true,
                        const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalAudioTrackImpl();
    // impl. of LocalTrack
    TrackSource source() const final;
    void fillRequest(AddTrackRequest* request) const final;
protected:
    // impl. of AudioTrackImpl
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack() const { return mediaTrack(); }
    // impl. LocalTrackImpl
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createMediaTrack(const std::string& id) final;
    void requestAuthorization() final;
private:
    const bool _microphone;
};

} // namespace LiveKitCpp
