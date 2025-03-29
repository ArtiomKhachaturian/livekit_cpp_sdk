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
#include "CameraVideoDevice.h"

namespace LiveKitCpp
{

CameraVideoDevice::CameraVideoDevice(const std::string& id,
                                     std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                     const webrtc::VideoCaptureCapability& initialCapability,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : _id(id)
    , _source(webrtc::make_ref_counted<CameraVideoSource>(std::move(signalingQueue),
                                                          initialCapability, logger))
{
}

CameraVideoDevice::~CameraVideoDevice()
{
    close();
}

void CameraVideoDevice::close()
{
    _source->close();
}

void CameraVideoDevice::setDeviceInfo(const MediaDeviceInfo& info)
{
    _source->setDeviceInfo(info);
}

MediaDeviceInfo CameraVideoDevice::deviceInfo() const
{
    return _source->deviceInfo();
}

void CameraVideoDevice::setCapability(const webrtc::VideoCaptureCapability& capability)
{
    _source->setCapability(capability);
}

webrtc::VideoCaptureCapability CameraVideoDevice::capability() const
{
    return _source->capability();
}

void CameraVideoDevice::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                        const rtc::VideoSinkWants& wants)
{
    _source->AddOrUpdateSink(sink, wants);
}

void CameraVideoDevice::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    _source->RemoveSink(sink);
}

webrtc::VideoTrackSourceInterface* CameraVideoDevice::GetSource() const
{
    return _source.get();
}

bool CameraVideoDevice::enabled() const
{
    return _source->enabled();
}

bool CameraVideoDevice::set_enabled(bool enable)
{
    return _source->setEnabled(enable);
}

webrtc::MediaStreamTrackInterface::TrackState CameraVideoDevice::state() const
{
    switch (_source->state()) {
        case webrtc::MediaSourceInterface::kEnded:
            return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
        default:
            break;
    }
    return webrtc::MediaStreamTrackInterface::TrackState::kLive;
}

void CameraVideoDevice::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _source->RegisterObserver(observer);
}

void CameraVideoDevice::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _source->UnregisterObserver(observer);
}

} // namespace LiveKitCpp
