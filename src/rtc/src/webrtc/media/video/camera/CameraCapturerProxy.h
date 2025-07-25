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
#include "CapturerObserver.h"
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
class CapturerProxySink;

class CameraCapturerProxy : private CapturerObserver,
                            private webrtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    ~CameraCapturerProxy() override;
    static std::shared_ptr<CameraCapturerProxy> create(webrtc::scoped_refptr<CameraCapturer> impl);
    bool started() const;
    bool captureCapability(webrtc::VideoCaptureCapability& capability) const;
    int32_t startCapture(const webrtc::VideoCaptureCapability& capability, CapturerProxySink* sink);
    int32_t stopCapture(CapturerProxySink* sink);
    const char* currentDeviceName() const;
protected:
    CameraCapturerProxy(webrtc::scoped_refptr<CameraCapturer> impl);
private:
    // impl. of CameraObserver
    void onStateChanged(CapturerState state) final;
    // impl. of webrtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints) final;
private:
    const webrtc::scoped_refptr<CameraCapturer> _impl;
    Bricks::Listeners<CapturerProxySink*> _sinks;
    Bricks::SafeOptional<webrtc::VideoCaptureCapability> _activeCapability;
};

} // LiveKitCpp
