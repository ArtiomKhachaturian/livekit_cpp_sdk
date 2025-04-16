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

CameraTrackImpl::CameraTrackImpl(EncryptionType encryption,
                                 std::shared_ptr<CameraDeviceImpl> cameraDevice,
                                 const std::weak_ptr<TrackManager>& trackManager)
    : Base("camera", encryption, std::move(cameraDevice), trackManager)
{
}

CameraTrackImpl::~CameraTrackImpl()
{
    CameraTrackImpl::close();
}

void CameraTrackImpl::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto t = track()) {
        t->setDeviceInfo(info);
    }
}

MediaDeviceInfo CameraTrackImpl::deviceInfo() const
{
    if (const auto t = track()) {
        return t->deviceInfo();
    }
    return {};
}

void CameraTrackImpl::setOptions(const CameraOptions& options)
{
    if (const auto t = track()) {
        t->setCapability(map(options));
    }
}

CameraOptions CameraTrackImpl::options() const
{
    if (const auto t = track()) {
        return map(t->capability());
    }
    return {};
}

void CameraTrackImpl::close()
{
    Base::close();
    if (const auto t = track()) {
        t->close();
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

webrtc::scoped_refptr<LocalCamera> CameraTrackImpl::track() const
{
    if (const auto& md = mediaDevice()) {
        return md->track();
    }
    return {};
}

} // namespace LiveKitCpp
