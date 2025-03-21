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
#pragma once
#include "CameraObserver.h"
#include "Listeners.h"
#include "SafeObjAliases.h"
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <modules/video_capture/video_capture_defines.h>
#include <memory>
#include <optional>

namespace LiveKitCpp
{

class CameraCapturer;
class CameraCapturerProxySink;

class CameraCapturerProxy : private CameraObserver,
                            private rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    ~CameraCapturerProxy() override;
    static std::shared_ptr<CameraCapturerProxy> create(rtc::scoped_refptr<CameraCapturer> impl);
    bool started() const;
    bool captureCapability(webrtc::VideoCaptureCapability& capability) const;
    int32_t startCapture(const webrtc::VideoCaptureCapability& capability, CameraCapturerProxySink* sink);
    int32_t stopCapture(CameraCapturerProxySink* sink);
    const char* currentDeviceName() const;
protected:
    CameraCapturerProxy(rtc::scoped_refptr<CameraCapturer> impl);
private:
    // impl. of CameraObserver
    void onStateChanged(CameraState state) final;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) final;
private:
    const rtc::scoped_refptr<CameraCapturer> _impl;
    Bricks::Listeners<CameraCapturerProxySink*> _sinks;
    Bricks::SafeOptional<webrtc::VideoCaptureCapability> _activeCapability;
};

} // LiveKitCpp
