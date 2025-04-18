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
#pragma once // LocalVideoTrackImpl.h
#include "LocalTrackImpl.h"
#include "LocalVideoDeviceImpl.h"
#include "VideoTrackImpl.h"
#include "livekit/rtc/media/LocalVideoTrack.h"

namespace LiveKitCpp
{

class LocalVideoTrackImpl : public LocalTrackImpl<VideoTrackImpl<LocalVideoDeviceImpl, LocalVideoTrack>>
{
    using Base = LocalTrackImpl<VideoTrackImpl<LocalVideoDeviceImpl, LocalVideoTrack>>;
public:
    LocalVideoTrackImpl(EncryptionType encryption,
                        std::shared_ptr<LocalVideoDeviceImpl> device,
                        const std::weak_ptr<TrackManager>& trackManager);
    // impl. of LocalTrack
    TrackSource source() const final;
    bool fillRequest(AddTrackRequest* request) const final;
    // impl. of LocalVideoTrack
    void setDeviceInfo(MediaDeviceInfo info) final;
    MediaDeviceInfo deviceInfo() const final;
    void setOptions(VideoOptions options) final;
    VideoOptions options() const final;
};
	
} // namespace LiveKitCpp
