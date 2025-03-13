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
#include "CameraTrack.h"
#include "MediaAuthorization.h"

namespace LiveKitCpp
{

CameraTrack::CameraTrack(LocalTrackManager* manager, const std::shared_ptr<Bricks::Logger>& logger)
    : LocalVideoTrack<CameraVideoTrack>("camera", manager, logger)
{
}

void CameraTrack::setDevice(MediaDevice device)
{
    if (const auto track = mediaTrack()) {
        track->setDevice(std::move(device));
    }
}

void CameraTrack::setCapability(webrtc::VideoCaptureCapability capability)
{
    if (const auto track = mediaTrack()) {
        track->setCapability(std::move(capability));
    }
}

webrtc::scoped_refptr<CameraVideoTrack> CameraTrack::createMediaTrack(const std::string& id)
{
    if (const auto m = manager()) {
        return m->createCamera(id);
    }
    return {};
}

void CameraTrack::fillRequest(AddTrackRequest* request) const
{
    LocalVideoTrack<CameraVideoTrack>::fillRequest(request);
    if (request) {
        request->_source = TrackSource::Camera;
    }
}

void CameraTrack::requestAuthorization()
{
    LocalVideoTrack<CameraVideoTrack>::requestAuthorization();
    MediaAuthorization::query(MediaAuthorizationKind::Camera, true, logger());
}

} // namespace LiveKitCpp
