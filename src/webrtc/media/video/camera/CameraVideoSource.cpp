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
#include "CameraVideoSource.h"
#include "CameraVideoSourceImpl.h"
#include "ThreadUtils.h"

namespace LiveKitCpp
{

CameraVideoSource::CameraVideoSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                     const webrtc::VideoCaptureCapability& initialCapability,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(signalingQueue), logger, initialCapability)
{
}

CameraVideoSource::~CameraVideoSource()
{
    postToImpl(&CameraVideoSourceImpl::close);
}

void CameraVideoSource::setDevice(MediaDevice device)
{
    if (_impl->active()) {
        postToImpl(&CameraVideoSourceImpl::setDevice, std::move(device));
    }
}

MediaDevice CameraVideoSource::device() const
{
    return _impl->device();
}

void CameraVideoSource::setCapability(const webrtc::VideoCaptureCapability& capability)
{
    if (_impl->active()) {
        postToImpl(&CameraVideoSourceImpl::setCapability, capability);
    }
}

webrtc::VideoCaptureCapability CameraVideoSource::capability() const
{
    return _impl->capability();
}

bool CameraVideoSource::GetStats(Stats* stats)
{
    if (stats) {
        return _impl->stats(*stats);
    }
    return false;
}

void CameraVideoSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (_impl->active()) {
        postToImpl(&CameraVideoSourceImpl::processConstraints, c);
    }
}

void CameraVideoSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                        const rtc::VideoSinkWants& wants)
{
    if (_impl->addOrUpdateSink(sink, wants)) {
        postToImpl(&CameraVideoSourceImpl::requestCapturer);
    }
}

void CameraVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (_impl->removeSink(sink)) {
        postToImpl(&CameraVideoSourceImpl::resetCapturer);
    }
}

} // namespace LiveKitCpp
