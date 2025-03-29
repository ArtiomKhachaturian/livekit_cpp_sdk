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
#include "CameraDevice.h"
#include "CameraTrack.h"
#include "LocalTrackImpl.h"
#include "VideoTrackImpl.h"

namespace LiveKitCpp
{

class CameraTrackImpl : public LocalTrackImpl<VideoTrackImpl<CameraTrack, CameraDevice>>
{
    using Base = LocalTrackImpl<VideoTrackImpl<CameraTrack, CameraDevice>>;
public:
    CameraTrackImpl(webrtc::scoped_refptr<CameraDevice> cameraTrack,
                    TrackManager* manager,
                    const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraTrackImpl();
    // impl. of CameraTrack
    void setDeviceInfo(const MediaDeviceInfo& info) final;
    MediaDeviceInfo deviceInfo() const final;
    void setOptions(const CameraOptions& options) final;
    CameraOptions options() const final;
    // impl. of LocalTrack
    void close() final;
    bool fillRequest(AddTrackRequest* request) const final;
};

} // namespace LiveKitCpp
