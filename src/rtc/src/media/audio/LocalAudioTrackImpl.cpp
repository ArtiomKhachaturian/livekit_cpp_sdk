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
#include "LocalAudioTrackImpl.h"

namespace {

// see enum class TrackSource
inline std::string audioLabel(bool microphone) {
    return microphone ? "microphone" : "screen_share_audio";
}

}

namespace LiveKitCpp
{

LocalAudioTrackImpl::LocalAudioTrackImpl(std::shared_ptr<AudioDeviceImpl> audioDevice,
                                         EncryptionType encryption,
                                         webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver,
                                         const std::weak_ptr<TrackManager>& trackManager,
                                         bool disableRed,
                                         bool microphone)
    : Base(audioLabel(microphone), std::move(audioDevice), encryption, std::move(transceiver), trackManager)
    , _disableRed(disableRed)
    , _microphone(microphone)
{
}

std::vector<AudioTrackFeature> LocalAudioTrackImpl::features() const
{
    std::vector<AudioTrackFeature> features;
    features.reserve(4U);
    if (stereoRecording().value_or(false)) {
        features.push_back(AudioTrackFeature::Stereo);
    }
    if (const auto source = audioSource()) {
        const auto options = source->options();
        if (options.echo_cancellation.value_or(false)) {
            features.push_back(AudioTrackFeature::Echocancellation);
        }
        if (options.auto_gain_control.value_or(false)) {
            features.push_back(AudioTrackFeature::AutoGainControl);
        }
        if (options.noise_suppression.value_or(false)) {
            features.push_back(AudioTrackFeature::NoiseSuppression);
        }
    }
    return features.empty() ? Base::features() : features;
}

TrackSource LocalAudioTrackImpl::source() const
{
    return _microphone ? TrackSource::Microphone : TrackSource::ScreenShareAudio;
}

bool LocalAudioTrackImpl::fillRequest(AddTrackRequest* request) const
{
    if (Base::fillRequest(request)) {
        request->_type = type();
        request->_source = source();
        request->_stereo = stereoRecording().value_or(false);
        request->_disableRed = _disableRed;
        if (EncryptionType::None != request->_encryption) {
            // https://github.com/livekit/client-sdk-js/blob/main/src/room/participant/LocalParticipant.ts#L964
            request->_disableRed = true;
        }
        return true;
    }
    return false;
}

std::optional<bool> LocalAudioTrackImpl::stereoRecording() const
{
    if (const auto m = trackManager()) {
        return m->stereoRecording();
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
