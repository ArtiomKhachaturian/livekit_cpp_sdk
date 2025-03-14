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
#pragma once // CameraTrack.h
#include "LocalVideoTrackImpl.h"
#include "CameraVideoTrack.h"
#include "CameraTrack.h"

namespace LiveKitCpp
{

class CameraTrackImpl : public LocalVideoTrackImpl<CameraVideoTrack, CameraTrack>
{
    using Base = LocalVideoTrackImpl<CameraVideoTrack, CameraTrack>;
public:
    CameraTrackImpl(LocalTrackManager* manager, const std::shared_ptr<Bricks::Logger>& logger = {});
    void setCapability(webrtc::VideoCaptureCapability capability);
    // impl. of CameraTrack
    void setDevice(MediaDevice device = {}) final;
    // impl. of LocalTrack
    void fillRequest(AddTrackRequest* request) const final;
protected:
    webrtc::scoped_refptr<CameraVideoTrack> createMediaTrack(const std::string& id) final;
    void requestAuthorization() final;
};

} // namespace LiveKitCpp
