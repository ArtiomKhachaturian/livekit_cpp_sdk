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
#include "MicAudioSource.h"
#include "AdmProxy.h"

namespace LiveKitCpp
{

MicAudioSource::MicAudioSource(rtc::WeakPtr<AdmProxy> admProxyRef,
                               cricket::AudioOptions options,
                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(signalingQueue), logger, true)
    , _admProxyRef(std::move(admProxyRef))
    , _options(std::move(options))
{
}

MicAudioSource::MicAudioSource(const webrtc::scoped_refptr<AdmProxy>& adm,
                               cricket::AudioOptions options,
                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue)
    : MicAudioSource(adm ? adm->weakRef() : rtc::WeakPtr<AdmProxy>{},
                     std::move(options), std::move(signalingQueue),
                     adm ? adm->logger() : std::shared_ptr<Bricks::Logger>{})
{
}

MicAudioSource::~MicAudioSource()
{
    postToImpl(&AudioSourceImpl::close);
}

void MicAudioSource::SetVolume(double volume)
{
    postToImpl(&AudioSourceImpl::setVolume, volume);
}

void MicAudioSource::RegisterAudioObserver(AudioObserver* observer)
{
    _impl->registerAudioObserver(observer);
}

void MicAudioSource::UnregisterAudioObserver(AudioObserver* observer)
{
    _impl->UnregisterAudioObserver(observer);
}

void MicAudioSource::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    _impl->addSink(sink);
}

void MicAudioSource::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    _impl->removeSink(sink);
}

const cricket::AudioOptions MicAudioSource::options() const
{
    return _options;
}

} // namespace LiveKitCpp
