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
#include "AudioDeviceImpl.h"

namespace LiveKitCpp
{

AudioDeviceImpl::AudioDeviceImpl(webrtc::scoped_refptr<webrtc::AudioTrackInterface> track)
    : Base(std::move(track))
{
}

AudioDeviceImpl::~AudioDeviceImpl()
{
    if (_sinks.clear()) {
        if (const auto& t = track()) {
            t->RemoveSink(&_sinks);
        }
    }
}

void AudioDeviceImpl::addSink(AudioSink* sink)
{
    const auto& t = track();
    if (t && Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        t->AddSink(&_sinks);
    }
}

void AudioDeviceImpl::removeSink(AudioSink* sink)
{
    const auto& t = track();
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink) && t) {
        t->RemoveSink(&_sinks);
    }
}

void AudioDeviceImpl::setVolume(double volume)
{
    if (const auto& t = track()) {
        if (const auto source = t->GetSource()) {
            source->SetVolume(volume);
        }
    }
}

std::optional<int> AudioDeviceImpl::signalLevel() const
{
    if (const auto& t = track()) {
        int level;
        if (t->GetSignalLevel(&level)) {
            return level;
        }
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
