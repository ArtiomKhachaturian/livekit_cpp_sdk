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
#include "CameraSource.h"
#include "AsyncCameraSourceImpl.h"
#include "ThreadUtils.h"

namespace LiveKitCpp
{

CameraSource::CameraSource(const std::string& id,
                           std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                           const MediaDeviceInfo& info,
                           const webrtc::VideoCaptureCapability& initialCapability,
                           const std::shared_ptr<Bricks::Logger>& logger)
    : Base(std::move(signalingQueue), logger, id, info, initialCapability)
{
}

CameraSource::~CameraSource()
{
    postToImpl(&AsyncCameraSourceImpl::close);
}

void CameraSource::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (_impl->active()) {
        postToImpl(&AsyncCameraSourceImpl::setDeviceInfo, info);
    }
}

MediaDeviceInfo CameraSource::deviceInfo() const
{
    return _impl->deviceInfo();
}

void CameraSource::setCapability(const webrtc::VideoCaptureCapability& capability)
{
    if (_impl->active()) {
        postToImpl(&AsyncCameraSourceImpl::setCapability, capability);
    }
}

webrtc::VideoCaptureCapability CameraSource::capability() const
{
    return _impl->capability();
}

void CameraSource::addListener(CameraEventsListener* listener)
{
    _impl->addListener(listener);
}

void CameraSource::removeListener(CameraEventsListener* listener)
{
    _impl->removeListener(listener);
}

const std::string& CameraSource::id() const
{
    return _impl->id();
}

webrtc::VideoTrackInterface::ContentHint CameraSource::contentHint() const
{
    return _impl->contentHint();
}

void CameraSource::setContentHint(webrtc::VideoTrackInterface::ContentHint hint)
{
    _impl->setContentHint(hint);
}

bool CameraSource::GetStats(Stats* stats)
{
    if (stats) {
        return _impl->stats(*stats);
    }
    return false;
}

void CameraSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    if (_impl->active()) {
        postToImpl(&AsyncCameraSourceImpl::processConstraints, c);
    }
}

void CameraSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                   const rtc::VideoSinkWants& wants)
{
    if (_impl->addOrUpdateSink(sink, wants)) {
        postToImpl(&AsyncCameraSourceImpl::requestCapturer);
    }
}

void CameraSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (_impl->removeSink(sink)) {
        postToImpl(&AsyncCameraSourceImpl::resetCapturer);
    }
}

} // namespace LiveKitCpp
