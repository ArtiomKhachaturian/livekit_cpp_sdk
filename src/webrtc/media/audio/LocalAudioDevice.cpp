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
#include "LocalAudioDevice.h"

namespace LiveKitCpp
{

LocalAudioDevice::LocalAudioDevice(const std::string& id,
                                   std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                   cricket::AudioOptions options,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : _id(id)
    , _source(webrtc::make_ref_counted<LocalAudioSource>(std::move(options),
                                                         std::move(signalingQueue),
                                                         logger))
{
}

webrtc::AudioSourceInterface* LocalAudioDevice::GetSource() const
{
    return _source.get();
}

void LocalAudioDevice::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    _source->AddSink(sink);
}

void LocalAudioDevice::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    _source->RemoveSink(sink);
}

bool LocalAudioDevice::GetSignalLevel(int* level)
{
    if (level) {
        
    }
    return false;
}

rtc::scoped_refptr<webrtc::AudioProcessorInterface> LocalAudioDevice::GetAudioProcessor()
{
    return nullptr;
}

bool LocalAudioDevice::enabled() const
{
    return _source->enabled();
}

bool LocalAudioDevice::set_enabled(bool enable)
{
    return _source->setEnabled(enable);
}

webrtc::MediaStreamTrackInterface::TrackState LocalAudioDevice::state() const
{
    switch (_source->state()) {
        case webrtc::MediaSourceInterface::kEnded:
            return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
        default:
            break;
    }
    return webrtc::MediaStreamTrackInterface::TrackState::kLive;
}

void LocalAudioDevice::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _source->RegisterObserver(observer);
}

void LocalAudioDevice::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _source->UnregisterObserver(observer);
}

} // namespace LiveKitCpp
