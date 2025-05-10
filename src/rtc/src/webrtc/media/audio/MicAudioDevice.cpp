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
#include "MicAudioDevice.h"
#include "LocalAudioRecorder.h"
#include "AsyncMicSourceImpl.h"
#include "PeerConnectionFactory.h"
#include "livekit/rtc/media/AudioRecordingOptions.h"
#include <api/audio_options.h>

namespace
{

inline cricket::AudioOptions toCricketOptions(const LiveKitCpp::AudioRecordingOptions& options)
{
    cricket::AudioOptions audioOptions;
    audioOptions.echo_cancellation = options._echoCancellation;
    audioOptions.auto_gain_control = options._autoGainControl;
    audioOptions.noise_suppression = options._noiseSuppression;
    audioOptions.highpass_filter = options._highpassFilter;
    audioOptions.stereo_swapping = options._stereoSwapping;
    return audioOptions;
}

}

namespace LiveKitCpp
{

MicAudioDevice::MicAudioDevice(const webrtc::scoped_refptr<ListenedAudio>& track)
    : AudioDeviceImpl(track)
{
    if (track) {
        track->addListener(this);
    }
}

MicAudioDevice::~MicAudioDevice()
{
    if (const auto t = dynamic_cast<ListenedAudio*>(track().get())) {
        t->removeListener(this);
        if (webrtc::MediaStreamTrackInterface::kLive == t->state()) {
            onMediaStopped();
        }
    }
}

std::unique_ptr<MicAudioDevice> MicAudioDevice::
    create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
           const AudioRecordingOptions& options,
           std::weak_ptr<AdmProxyFacade> admProxy,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    std::unique_ptr<MicAudioDevice> device;
    if (!admProxy.expired()) {
        using TrackType = LocalAudioRecorder<AsyncMicSourceImpl>;
        auto track = TrackType::create(std::move(signalingQueue),
                                       logger,
                                       toCricketOptions(options),
                                       std::move(admProxy));
        if (track) {
            device.reset(new MicAudioDevice(std::move(track)));
        }
    }
    return device;
}

std::unique_ptr<MicAudioDevice> MicAudioDevice::create(const PeerConnectionFactory* pcf,
                                                       const AudioRecordingOptions& options,
                                                       const std::shared_ptr<Bricks::Logger>& logger)
{
    if (pcf) {
        return create(pcf->signalingThread(), options, pcf->admProxy(), logger);
    }
    return {};
}

} // namespace LiveKitCpp
