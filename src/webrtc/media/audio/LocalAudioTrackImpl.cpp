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

LocalAudioTrackImpl::LocalAudioTrackImpl(webrtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack,
                                         TrackManager* manager, bool microphone,
                                         const std::shared_ptr<Bricks::Logger>& logger)
    : Base(audioLabel(microphone), std::move(audioTrack), manager, logger)
    , _microphone(microphone)
{
    installSink(true, audioSink());
}

LocalAudioTrackImpl::~LocalAudioTrackImpl()
{
    installSink(false, audioSink());
}

void LocalAudioTrackImpl::setVolume(double volume)
{
    if (const auto source = audioSource()) {
        source->SetVolume(volume);
    }
}

std::vector<AudioTrackFeature> LocalAudioTrackImpl::features() const
{
    std::vector<AudioTrackFeature> features;
    features.reserve(4U);
    if (const auto m = manager()) {
        if (m->stereoRecording().value_or(false)) {
            features.push_back(AudioTrackFeature::Stereo);
        }
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
        return true;
    }
    return false;
}

void LocalAudioTrackImpl::installSink(bool install, webrtc::AudioTrackSinkInterface* sink)
{
    if (sink) {
        installSink(install, sink, mediaTrack());
    }
}

bool LocalAudioTrackImpl::signalLevel(int& level) const
{
    if (const auto& track = mediaTrack()) {
        return track->GetSignalLevel(&level);
    }
    return false;
}

webrtc::AudioSourceInterface* LocalAudioTrackImpl::audioSource() const
{
    if (const auto& track = mediaTrack()) {
        return track->GetSource();
    }
    return nullptr;
}

void LocalAudioTrackImpl::installSink(bool install, webrtc::AudioTrackSinkInterface* sink,
                                      const webrtc::scoped_refptr<webrtc::AudioTrackInterface>& track)
{
    if (sink && track) {
        if (install) {
            track->AddSink(sink);
        }
        else {
            track->RemoveSink(sink);
        }
    }
}

} // namespace LiveKitCpp
