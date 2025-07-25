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
#include "VideoDeviceImpl.h"
#include "VideoUtils.h"

namespace LiveKitCpp
{

VideoDeviceImpl::VideoDeviceImpl(webrtc::scoped_refptr<webrtc::VideoTrackInterface> track)
    : Base(std::move(track))
{
}

VideoDeviceImpl::~VideoDeviceImpl()
{
    if (_sinks.clear()) {
        if (const auto& t = track()) {
            t->RemoveSink(&_sinks);
        }
    }
}

void VideoDeviceImpl::addSink(VideoSink* sink)
{
    const auto& t = track();
    if (t && Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        t->AddOrUpdateSink(&_sinks, {});
    }
}

void VideoDeviceImpl::removeSink(VideoSink* sink)
{
    const auto& t = track();
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink) && t) {
        t->RemoveSink(&_sinks);
    }
}

void VideoDeviceImpl::setContentHint(VideoContentHint hint)
{
    if (const auto& t = track()) {
        t->set_content_hint(map(hint));
    }
}

VideoContentHint VideoDeviceImpl::contentHint() const
{
    if (const auto& t = track()) {
        return map(t->content_hint());
    }
    return VideoDevice::contentHint();
}

} // namespace LiveKitCpp
