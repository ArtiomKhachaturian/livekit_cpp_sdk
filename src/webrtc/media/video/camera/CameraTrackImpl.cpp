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
#include "CameraTrackImpl.h"
#include "CameraManager.h"

namespace LiveKitCpp
{

CameraTrackImpl::CameraTrackImpl(webrtc::scoped_refptr<CameraVideoTrack> cameraTrack,
                                 TrackManager* manager,
                                 const std::shared_ptr<Bricks::Logger>& logger)
    : Base("camera", std::move(cameraTrack), manager, logger)
{
    installSink(true, videoSink());
}

CameraTrackImpl::~CameraTrackImpl()
{
    installSink(false, videoSink());
    CameraTrackImpl::close();
}

void CameraTrackImpl::setDevice(MediaDevice device)
{
    if (const auto& track = mediaTrack()) {
        track->setDevice(std::move(device));
    }
}

MediaDevice CameraTrackImpl::device() const
{
    if (const auto& track = mediaTrack()) {
        return track->device();
    }
    return {};
}

void CameraTrackImpl::setOptions(const CameraOptions& options)
{
    if (const auto& track = mediaTrack()) {
        track->setCapability(map(options));
    }
}

CameraOptions CameraTrackImpl::options() const
{
    if (const auto& track = mediaTrack()) {
        return map(track->capability());
    }
    return {};
}

void CameraTrackImpl::close()
{
    Base::close();
    if (const auto& track = mediaTrack()) {
        track->close();
    }
}

bool CameraTrackImpl::fillRequest(AddTrackRequest* request) const
{
    if (Base::fillRequest(request)) {
        request->_type = type();
        request->_source = source();
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
