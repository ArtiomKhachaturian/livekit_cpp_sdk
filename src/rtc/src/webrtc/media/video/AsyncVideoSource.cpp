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
    return _impl ? _impl->deviceInfo() : MediaDeviceInfo{};
}

void AsyncVideoSource::setOptions(VideoOptions options)
{
    if (active()) {
        postToImpl(&AsyncVideoSourceImpl::setOptions, std::move(options));
    }
}

VideoOptions AsyncVideoSource::options() const
{
    return _impl ? _impl->options() : VideoOptions{};
}

void AsyncVideoSource::addListener(MediaDeviceListener* listener)
{
    if (_impl) {
        _impl->addListener(listener);
    }
}

void AsyncVideoSource::removeListener(MediaDeviceListener* listener)
{
    if (_impl) {
        _impl->removeListener(listener);
    }
}

void AsyncVideoSource::setFilter(LocalVideoFilterPin* inputPin)
{
    if (_impl) {
        _impl->setFilter(inputPin);
    }
}

VideoContentHint AsyncVideoSource::contentHint() const
{
    return _impl ? _impl->contentHint() : VideoContentHint::None;
}

void AsyncVideoSource::setContentHint(VideoContentHint hint)
{
    if (active() && _impl->changeContentHint(hint)) {
        postToImpl(&AsyncVideoSourceImpl::updateAfterContentHintChanges, hint);
    }
}

bool AsyncVideoSource::GetStats(webrtc::VideoTrackSourceInterface::Stats* stats)
{
    return stats && _impl && _impl->stats(*stats);
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
    if (_impl && _impl->addOrUpdateSink(sink, wants)) {
        postToImpl(&AsyncVideoSourceImpl::requestCapturer);
    }
}

void AsyncVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (_impl && _impl->removeSink(sink)) {
        postToImpl(&AsyncVideoSourceImpl::resetCapturer);
    }
}

} // namespace LiveKitCpp
