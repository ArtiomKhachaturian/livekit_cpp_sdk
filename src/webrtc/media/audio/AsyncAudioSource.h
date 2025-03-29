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
#include "LocalAudioSource.h"

namespace LiveKitCpp
{

template<class TAsyncImpl>
class AsyncAudioSource : public AsyncMediaSource<LocalAudioSource, TAsyncImpl>
{
    static_assert(std::is_base_of_v<AsyncAudioSourceImpl, TAsyncImpl>);
    using Base = AsyncMediaSource<LocalAudioSource, TAsyncImpl>;
public:
    // impl. of LocalAudioSource
    bool setAudioEnabled(bool enabled) final { return Base::setEnabled(enabled); }
    bool audioEnabled() const final { return Base::enabled(); }
    // impl. of webrtc::AudioSourceInterface
    void SetVolume(double volume) override;
    void RegisterAudioObserver(LocalAudioSource::AudioObserver* observer) override;
    void UnregisterAudioObserver(LocalAudioSource::AudioObserver* observer) override;
    void AddSink(webrtc::AudioTrackSinkInterface*  sink) override;
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) override;
protected:
    template<typename... Args>
    AsyncAudioSource(Args&&... args);
};

template<class TAsyncImpl>
template<typename... Args>
inline AsyncAudioSource<TAsyncImpl>::AsyncAudioSource(Args&&... args)
    : Base(std::forward<Args>(args)...)
{
}

template<class TAsyncImpl>
void AsyncAudioSource<TAsyncImpl>::SetVolume(double volume)
{
    
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::RegisterAudioObserver(LocalAudioSource::AudioObserver* observer)
{
    Base::_impl->addAudioObserver(observer);
}

template<class TAsyncImpl>
inline void AsyncAudioSource<TAsyncImpl>::UnregisterAudioObserver(LocalAudioSource::AudioObserver* observer)
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

} // namespace LiveKitCpp
