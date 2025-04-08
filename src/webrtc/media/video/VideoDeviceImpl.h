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
#pragma once // VideoDeviceImpl.h
#include "media/VideoDevice.h"
#include "MediaDeviceImpl.h"
#include "VideoSinks.h"

namespace LiveKitCpp
{

class VideoDeviceImpl : public MediaDeviceImpl<webrtc::VideoTrackInterface, VideoDevice>
{
    using Base = MediaDeviceImpl<webrtc::VideoTrackInterface, VideoDevice>;
public:
    VideoDeviceImpl(webrtc::scoped_refptr<webrtc::VideoTrackInterface> track);
    ~VideoDeviceImpl() override;
    // impl. of MediaDevice
    bool audio() const final { return false; }
    // impl. of VideoDevice
    void addSink(VideoSink* sink) final;
    void removeSink(VideoSink* sink) final;
private:
    VideoSinks _sinks;
};

} // namespace LiveKitCpp
