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
#pragma once // AsyncAudioSource.h
#include "AsyncMediaSource.h"
#include "AsyncAudioSourceImpl.h"

namespace LiveKitCpp
{

template<class TAsyncImpl>
class AsyncAudioSource : public AsyncMediaSource<webrtc::AudioSourceInterface, TAsyncImpl>
{
    static_assert(std::is_base_of_v<AsyncAudioSourceImpl, TAsyncImpl>);
    using Base = AsyncMediaSource<webrtc::AudioSourceInterface, TAsyncImpl>;
public:
    template<typename... Args>
    AsyncAudioSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args);
    // impl. of webrtc::AudioSourceInterface
    void SetVolume(double volume) final;
    void RegisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer) final;
    void UnregisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer) final;
    void AddSink(webrtc::AudioTrackSinkInterface*  sink) final;
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) final;
    const cricket::AudioOptions options() const final;
};

template<class TAsyncImpl>
template<typename... Args>
inline AsyncAudioSource<TAsyncImpl>::AsyncAudioSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                      const std::shared_ptr<Bricks::Logger>& logger,
                                                      Args&&... args)
    : Base(std::move(signalingQueue), logger, std::forward<Args>(args)...)
{
}

template<class TAsyncImpl>
void AsyncAudioSource<TAsyncImpl>::SetVolume(double volume)
{
    Base::postToImpl(&AsyncAudioSourceImpl::setVolume, volume);
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::
    RegisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer)
{
    Base::_impl->addAudioObserver(observer);
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::
    UnregisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer)
{
    Base::_impl->removeAudioObserver(observer);
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::AddSink(webrtc::AudioTrackSinkInterface*  sink)
{
    Base::_impl->addSink(sink);
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    Base::_impl->removeSink(sink);
}

template<class TAsyncImpl>
inline const cricket::AudioOptions AsyncAudioSource<TAsyncImpl>::options() const
{
    return Base::_impl->options();
}

} // namespace LiveKitCpp
