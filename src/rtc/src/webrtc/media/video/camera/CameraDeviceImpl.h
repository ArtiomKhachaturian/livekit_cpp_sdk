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
#pragma once // CameraDeviceImpl.h
#include "LocalCamera.h"
#include "MediaDeviceImpl.h"
#include "VideoSinks.h"
#include "livekit/rtc/media/CameraDevice.h"
#include <memory>

namespace webrtc {
class TaskQueueBase;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;

class CameraDeviceImpl : public MediaDeviceImpl<LocalCamera, CameraDevice>
{
    using Base = MediaDeviceImpl<LocalCamera, CameraDevice>;
public:
    static std::shared_ptr<CameraDeviceImpl> create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                    const MediaDeviceInfo& info = {},
                                                    const CameraOptions& options = {},
                                                    const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::shared_ptr<CameraDeviceImpl> create(const PeerConnectionFactory* pcf,
                                                    const MediaDeviceInfo& info = {},
                                                    const CameraOptions& options = {},
                                                    const std::shared_ptr<Bricks::Logger>& logger = {});
    ~CameraDeviceImpl() final;
    // impl. of MediaDevice
    bool audio() const final { return false; }
    // overrides of MediaDeviceImpl<>
    bool addListener(MediaEventsListener* listener) final;
    bool removeListener(MediaEventsListener* listener) final;
    // impl. of VideoDevice
    void addSink(VideoSink* sink) final;
    void removeSink(VideoSink* sink) final;
    // impl. of CameraDevice
    void setDeviceInfo(const MediaDeviceInfo& info) final;
    MediaDeviceInfo deviceInfo() const final;
    void setOptions(const CameraOptions& options) final;
    CameraOptions options() const final;
private:
    CameraDeviceImpl(webrtc::scoped_refptr<LocalCamera> track);
private:
    VideoSinks _sinks;
};

} // namespace LiveKitCpp
