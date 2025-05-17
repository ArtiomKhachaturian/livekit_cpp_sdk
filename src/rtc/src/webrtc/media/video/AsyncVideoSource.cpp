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
#include "AsyncVideoSource.h"

namespace LiveKitCpp
{

AsyncVideoSource::AsyncVideoSource(std::shared_ptr<AsyncVideoSourceImpl> impl)
    : Base(std::move(impl))
{
}

void AsyncVideoSource::setDeviceInfo(MediaDeviceInfo info)
{
    if (active()) {
        postToImpl(&AsyncVideoSourceImpl::setDeviceInfo, std::move(info));
    }
}

MediaDeviceInfo AsyncVideoSource::deviceInfo() const
{
    if (const auto impl = loadImpl()) {
        return impl->deviceInfo();
    }
    return {};
}

void AsyncVideoSource::setOptions(VideoOptions options)
{
    if (active()) {
        postToImpl(&AsyncVideoSourceImpl::setOptions, std::move(options));
    }
}

VideoOptions AsyncVideoSource::options() const
{
    if (const auto impl = loadImpl()) {
        return impl->options();
    }
    return {};
}

void AsyncVideoSource::addListener(MediaDeviceListener* listener)
{
    if (const auto impl = loadImpl()) {
        impl->addListener(listener);
    }
}

void AsyncVideoSource::removeListener(MediaDeviceListener* listener)
{
    if (const auto impl = loadImpl()) {
        impl->removeListener(listener);
    }
}

void AsyncVideoSource::setFilter(LocalVideoFilterPin* inputPin)
{
    if (const auto impl = loadImpl()) {
        impl->setFilter(inputPin);
    }
}

VideoContentHint AsyncVideoSource::contentHint() const
{
    if (const auto impl = loadImpl()) {
        return impl->contentHint();
    }
    return VideoContentHint::None;
}

void AsyncVideoSource::setContentHint(VideoContentHint hint)
{
    const auto impl = loadImpl();
    if (impl && impl->active() && impl->changeContentHint(hint)) {
        postToImpl(&AsyncVideoSourceImpl::updateAfterContentHintChanges, hint);
    }
}

bool AsyncVideoSource::GetStats(webrtc::VideoTrackSourceInterface::Stats* stats)
{
    if (stats) {
        if (const auto impl = loadImpl()) {
            return impl->stats(*stats);
        }
    }
    return false;
}

void AsyncVideoSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        postToImpl(&AsyncVideoSourceImpl::processConstraints, c);
    }
}

void AsyncVideoSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                       const rtc::VideoSinkWants& wants)
{
    if (sink) {
        const auto impl = loadImpl();
        if (impl && impl->addOrUpdateSink(sink, wants)) {
            postToImpl(&AsyncVideoSourceImpl::requestCapturer);
        }
    }
}

void AsyncVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        const auto impl = loadImpl();
        if (impl && impl->removeSink(sink)) {
            postToImpl(&AsyncVideoSourceImpl::resetCapturer);
        }
    }
}

} // namespace LiveKitCpp
