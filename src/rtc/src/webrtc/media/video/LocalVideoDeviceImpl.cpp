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
#include "LocalVideoDeviceImpl.h"
#include "VideoUtils.h"

namespace LiveKitCpp
{

LocalVideoDeviceImpl::LocalVideoDeviceImpl(webrtc::scoped_refptr<LocalWebRtcTrack> track)
    : Base(std::move(track))
{
    if (const auto& t = this->track()) {
        t->addListener(this);
    }
}

LocalVideoDeviceImpl::~LocalVideoDeviceImpl()
{
    if (const auto& t = track()) {
        if (_sinks.clear()) {
            t->RemoveSink(&_sinks);
        }
        t->removeListener(this);
        if (webrtc::MediaStreamTrackInterface::kLive == t->state()) {
            onMediaStopped();
        }
    }
}

bool LocalVideoDeviceImpl::screencast() const
{
    const auto& t = track();
    return t && t->GetSource() && t->GetSource()->is_screencast();
}

void LocalVideoDeviceImpl::addSink(VideoSink* sink)
{
    const auto& t = track();
    if (t && Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        t->AddOrUpdateSink(&_sinks, {});
    }
}

void LocalVideoDeviceImpl::removeSink(VideoSink* sink)
{
    const auto& t = track();
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink) && t) {
        t->RemoveSink(&_sinks);
    }
}

void LocalVideoDeviceImpl::setContentHint(VideoContentHint hint)
{
    if (const auto& t = track()) {
        t->set_content_hint(map(hint));
    }
}

VideoContentHint LocalVideoDeviceImpl::contentHint() const
{
    if (const auto& t = track()) {
        return map(t->content_hint());
    }
    return VideoDevice::contentHint();
}

void LocalVideoDeviceImpl::setDeviceInfo(MediaDeviceInfo info)
{
    if (const auto& t = track()) {
        t->setDeviceInfo(std::move(info));
    }
}

MediaDeviceInfo LocalVideoDeviceImpl::deviceInfo() const
{
    if (const auto& t = track()) {
        return t->deviceInfo();
    }
    return {};
}

void LocalVideoDeviceImpl::setOptions(VideoOptions options)
{
    if (const auto& t = track()) {
        t->setOptions(std::move(options));
    }
}

VideoOptions LocalVideoDeviceImpl::options() const
{
    if (const auto& t = track()) {
        return t->options();
    }
    return {};
}

} // namespace LiveKitCpp
