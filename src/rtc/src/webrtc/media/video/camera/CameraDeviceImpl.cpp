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
#include "CameraDeviceImpl.h"
#include "CameraManager.h"
#include "PeerConnectionFactory.h"
#include "VideoFrameBuffer.h"
#include "Utils.h"

namespace LiveKitCpp
{

std::shared_ptr<CameraDeviceImpl> CameraDeviceImpl::
    create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
           const MediaDeviceInfo& info,
           const CameraOptions& options,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    std::shared_ptr<CameraDeviceImpl> device;
    if (!signalingQueue.expired()) {
        auto track = webrtc::make_ref_counted<LocalCamera>(makeUuid(),
                                                           std::move(signalingQueue),
                                                           info, map(options),
                                                           logger);
        device.reset(new CameraDeviceImpl(std::move(track)));
    }
    return device;
}

std::shared_ptr<CameraDeviceImpl> CameraDeviceImpl::create(const PeerConnectionFactory* pcf,
                                                           const MediaDeviceInfo& info,
                                                           const CameraOptions& options,
                                                           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (pcf) {
        return create(pcf->signalingThread(), info, options, logger);
    }
    return {};
}

CameraDeviceImpl::CameraDeviceImpl(webrtc::scoped_refptr<LocalCamera> track)
    : Base(std::move(track))
{
    if (const auto& t = this->track()) {
        t->addListener(this);
    }
}

CameraDeviceImpl::~CameraDeviceImpl()
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

void CameraDeviceImpl::addSink(VideoSink* sink)
{
    const auto& t = track();
    if (t && Bricks::AddResult::OkFirst == _sinks.add(sink)) {
        t->AddOrUpdateSink(&_sinks, {});
    }
}

void CameraDeviceImpl::removeSink(VideoSink* sink)
{
    const auto& t = track();
    if (Bricks::RemoveResult::OkLast == _sinks.remove(sink) && t) {
        t->RemoveSink(&_sinks);
    }
}

void CameraDeviceImpl::setContentHint(VideoContentHint hint)
{
    if (const auto& t = track()) {
        t->set_content_hint(map(hint));
    }
}

VideoContentHint CameraDeviceImpl::contentHint() const
{
    if (const auto& t = track()) {
        return map(t->content_hint());
    }
    return VideoDevice::contentHint();
}

void CameraDeviceImpl::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto& t = track()) {
        t->setDeviceInfo(info);
    }
}

MediaDeviceInfo CameraDeviceImpl::deviceInfo() const
{
    if (const auto& t = track()) {
        return t->deviceInfo();
    }
    return {};
}

void CameraDeviceImpl::setOptions(const CameraOptions& options)
{
    if (const auto& t = track()) {
        t->setCapability(map(options));
    }
}

CameraOptions CameraDeviceImpl::options() const
{
    if (const auto& t = track()) {
        return map(t->capability());
    }
    return {};
}

} // namespace LiveKitCpp
