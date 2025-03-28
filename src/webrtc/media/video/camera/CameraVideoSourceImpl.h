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
#pragma once // CameraVideoSourceImpl.h
#include "CameraCapturerProxySink.h"
#include "CameraCapturer.h"
#include "MediaDevice.h"
#include "SafeScopedRefPtr.h"
#include "VideoSourceImpl.h"

namespace LiveKitCpp
{

class CameraVideoSourceImpl : public VideoSourceImpl, private CameraCapturerProxySink
{
public:
    CameraVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                          const std::shared_ptr<Bricks::Logger>& logger,
                          const webrtc::VideoCaptureCapability& initialCapability);
    ~CameraVideoSourceImpl() final { close(); }
    MediaDevice device() const { return _device(); }
    void setDevice(MediaDevice device);
    void setCapability(webrtc::VideoCaptureCapability capability);
    webrtc::VideoCaptureCapability capability() const { return _capability(); }
    void requestCapturer();
    void resetCapturer();
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
    // impl. of MediaSourceImpl
    void onClosed() final;
    void onEnabled(bool enabled) final;
private:
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      std::string_view guid = {});
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      const rtc::scoped_refptr<CameraCapturer>& capturer);
    bool startCapturer(const webrtc::VideoCaptureCapability& capability); // non-threadsafe
    bool stopCapturer(bool sendByeFrame); // non-threadsafe
    void logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                  const std::string& message, int code = 0) const;
    void logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer, const std::string& message) const;
    // impl. of CameraObserver
    void onStateChanged(CameraState state) final;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final { broadcast(frame, true); }
    void OnDiscardedFrame() final { discard(); }
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c) final;
private:
    Bricks::SafeObj<MediaDevice> _device;
    SafeScopedRefPtr<CameraCapturer> _capturer;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _capability;
};

} // namespace LiveKitCpp
