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
#pragma once // LocalAudioDevice.h
#include "AsyncAudioSource.h"
#include "Utils.h"
#include <api/media_stream_interface.h>
#include <api/make_ref_counted.h>

namespace LiveKitCpp
{

class LocalAudioSource;

template<class TAsyncImpl>
class LocalAudioDevice : public webrtc::AudioTrackInterface
{
    using Source = AsyncAudioSource<TAsyncImpl>;
public:
    template<typename... Args>
    LocalAudioDevice(const std::string& id,
                     std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args);
    template<typename... Args>
    static webrtc::scoped_refptr<webrtc::AudioTrackInterface>
        create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
               const std::shared_ptr<Bricks::Logger>& logger,
               Args&&... args);
    // impl. of webrtc::AudioTrackInterface
    webrtc::AudioSourceInterface* GetSource() const final { return _source.get(); }
    void AddSink(webrtc::AudioTrackSinkInterface* sink) final;
    void RemoveSink(webrtc::AudioTrackSinkInterface* sink) final;
    bool GetSignalLevel(int* level) final;
    // impl. of MediaStreamTrackInterface
    std::string kind() const final { return webrtc::AudioTrackInterface::kAudioKind; }
    std::string id() const final { return _id; }
    bool enabled() const final { return _source->enabled(); }
    bool set_enabled(bool enable) final { return _source->setEnabled(enable); }
    webrtc::MediaStreamTrackInterface::TrackState state() const final;
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
private:
    const std::string _id;
    const webrtc::scoped_refptr<Source> _source;
};

template<class TAsyncImpl>
template<typename... Args>
inline LocalAudioDevice<TAsyncImpl>::LocalAudioDevice(const std::string& id,
                                                      std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                      const std::shared_ptr<Bricks::Logger>& logger,
                                                      Args&&... args)
    : _id(id)
    , _source(webrtc::make_ref_counted<Source>(std::move(signalingQueue),
                                               logger,
                                               std::forward<Args>(args)...))
{
}

template<class TAsyncImpl>
template<typename... Args>
inline webrtc::scoped_refptr<webrtc::AudioTrackInterface>
    LocalAudioDevice<TAsyncImpl>::create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                         const std::shared_ptr<Bricks::Logger>& logger,
                                         Args&&... args)
{
    using Type = LocalAudioDevice<TAsyncImpl>;
    return webrtc::make_ref_counted<Type>(makeUuid(),
                                          std::move(signalingQueue),
                                          logger,
                                          std::forward<Args>(args)...);
}

template<class TAsyncImpl>
void LocalAudioDevice<TAsyncImpl>::AddSink(webrtc::AudioTrackSinkInterface* sink)
{
    _source->AddSink(sink);
}

template<class TAsyncImpl>
void LocalAudioDevice<TAsyncImpl>::RemoveSink(webrtc::AudioTrackSinkInterface* sink)
{
    _source->RemoveSink(sink);
}

template<class TAsyncImpl>
bool LocalAudioDevice<TAsyncImpl>::GetSignalLevel(int* level)
{
    if (level) {
        
    }
    return false;
}

template<class TAsyncImpl>
webrtc::MediaStreamTrackInterface::TrackState LocalAudioDevice<TAsyncImpl>::state() const
{
    switch (_source->state()) {
        case webrtc::MediaSourceInterface::kEnded:
            return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
        default:
            break;
    }
    return webrtc::MediaStreamTrackInterface::TrackState::kLive;
}

template<class TAsyncImpl>
void LocalAudioDevice<TAsyncImpl>::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _source->RegisterObserver(observer);
}

template<class TAsyncImpl>
void LocalAudioDevice<TAsyncImpl>::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _source->UnregisterObserver(observer);
}

} // namespace LiveKitCpp
