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
#include "MicAudioSource.h"

namespace LiveKitCpp
{

MicAudioDevice::MicAudioDevice(const std::string& id, webrtc::scoped_refptr<MicAudioSource> source)
    : _id(id)
    , _source(std::move(source))
    , _enabled(_source && _source->enabled())
{
}

webrtc::AudioSourceInterface* MicAudioDevice::GetSource() const
{
    return _source.get();
}

void MicAudioDevice::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (_source) {
        _source->AddSink(sink);
    }
}

void MicAudioDevice::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (_source) {
        _source->RemoveSink(sink);
    }
}

bool MicAudioDevice::GetSignalLevel(int* level)
{
    if (level) {
        
    }
    return false;
}

bool MicAudioDevice::enabled() const
{
    return _enabled;
}

bool MicAudioDevice::set_enabled(bool enable)
{
    if (_source && enable != _enabled.exchange(enable)) {
        _source->setEnabled(enable);
        return true;
    }
    return false;
}

webrtc::MediaStreamTrackInterface::TrackState MicAudioDevice::state() const
{
    switch (_source->state()) {
        case webrtc::MediaSourceInterface::kEnded:
            return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
        default:
            break;
    }
    return webrtc::MediaStreamTrackInterface::TrackState::kLive;
}

void MicAudioDevice::RegisterObserver(webrtc::ObserverInterface* observer)
{
    //_source->RegisterObserver(observer);
}

void MicAudioDevice::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    //_source->UnregisterObserver(observer);
}

} // namespace LiveKitCpp
