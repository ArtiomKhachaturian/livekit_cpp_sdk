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
#pragma once // LocalVideoDeviceImpl.h
#include "livekit/rtc/media/LocalVideoDevice.h"
#include "LocalWebRtcTrack.h"
#include "MediaDeviceImpl.h"
#include "VideoSinks.h"

namespace LiveKitCpp
{

class LocalVideoDeviceImpl : public MediaDeviceImpl<LocalWebRtcTrack, LocalVideoDevice>
{
    using Base = MediaDeviceImpl<LocalWebRtcTrack, LocalVideoDevice>;
public:
    LocalVideoDeviceImpl(webrtc::scoped_refptr<LocalWebRtcTrack> track);
    ~LocalVideoDeviceImpl() override;
    bool screencast() const;
    // impl. of VideoDevice
    void addSink(VideoSink* sink) final;
    void removeSink(VideoSink* sink) final;
    void setContentHint(VideoContentHint hint) final;
    VideoContentHint contentHint() const final;
    // impl. of LocalVideoDevice
    void setDeviceInfo(MediaDeviceInfo info) final;
    MediaDeviceInfo deviceInfo() const final;
    void setOptions(VideoOptions options) final;
    VideoOptions options() const final;
private:
    VideoSinks _sinks;
};
	
} // namespace LiveKitCpp
