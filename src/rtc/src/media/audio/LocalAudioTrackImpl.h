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
#include "livekit/rtc/media/LocalAudioTrack.h"

namespace LiveKitCpp
{

class AudioDeviceImpl;

class LocalAudioTrackImpl : public LocalTrackImpl<AudioTrackImpl<LocalAudioTrack>>
{
    using Base = LocalTrackImpl<AudioTrackImpl<LocalAudioTrack>>;
public:
    LocalAudioTrackImpl(std::shared_ptr<AudioDeviceImpl> audioDevice,
                        EncryptionType encryption,
                        webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver,
                        const std::weak_ptr<TrackManager>& trackManager,
                        bool disableRed,
                        bool microphone = true);
    // impl. of AudioTrack
    std::vector<AudioTrackFeature> features() const final;
    // impl. of LocalTrack
    TrackSource source() const final;
    bool fillRequest(AddTrackRequest* request) const final;
private:
    std::optional<bool> stereoRecording() const;
private:
    const bool _disableRed;
    const bool _microphone;
};

} // namespace LiveKitCpp
