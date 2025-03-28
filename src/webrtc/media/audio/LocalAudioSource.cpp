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
#include "LocalAudioSource.h"

namespace LiveKitCpp
{

LocalAudioSource::LocalAudioSource(cricket::AudioOptions options,
                                   std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(signalingQueue), logger, true)
    , _options(std::move(options))
{
}

LocalAudioSource::~LocalAudioSource()
{
    postToImpl(&AudioSourceImpl::close);
}

void LocalAudioSource::SetVolume(double volume)
{
    postToImpl(&AudioSourceImpl::setVolume, volume);
}

void LocalAudioSource::RegisterAudioObserver(AudioObserver* observer)
{
    _impl->registerAudioObserver(observer);
}

void LocalAudioSource::UnregisterAudioObserver(AudioObserver* observer)
{
    _impl->UnregisterAudioObserver(observer);
}

void LocalAudioSource::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    _impl->addSink(sink);
}

void LocalAudioSource::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    _impl->removeSink(sink);
}

const cricket::AudioOptions LocalAudioSource::options() const
{
    return _options;
}

} // namespace LiveKitCpp
