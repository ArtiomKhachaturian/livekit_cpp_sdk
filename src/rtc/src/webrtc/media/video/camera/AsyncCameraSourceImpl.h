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
#pragma once // AsyncCameraSourceImpl.h
#include "CameraCapturer.h"
#include "SafeScopedRefPtr.h"
#include "AsyncVideoSourceImpl.h"

namespace LiveKitCpp
{

class CameraManager;

class AsyncCameraSourceImpl : public AsyncVideoSourceImpl
{
public:
    AsyncCameraSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                          std::weak_ptr<CameraManager> manager,
                          const std::shared_ptr<Bricks::Logger>& logger);
    ~AsyncCameraSourceImpl() final { close(); }
    // override of AsyncVideoSourceImpl
    bool changeContentHint(VideoContentHint hint) final;
    void requestCapturer() final;
    void resetCapturer() final;
    void updateAfterContentHintChanges(VideoContentHint hint) final;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
    // overrides of AsyncVideoSourceImpl
    void onCapturingError(std::string details, bool fatal) override;
    void onOptionsChanged(const VideoOptions& options) final;
    MediaDeviceInfo validate(MediaDeviceInfo info) const final;
    VideoOptions validate(VideoOptions options) const final;
private:
    webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                               const webrtc::scoped_refptr<CameraCapturer>& capturer) const;
    webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability) const;
    webrtc::scoped_refptr<CameraCapturer> create(const MediaDeviceInfo& dev) const;
    bool startCapturer(const webrtc::scoped_refptr<CameraCapturer>& capturer,
                       const webrtc::VideoCaptureCapability& capability) const;
    bool stopCapturer(const webrtc::scoped_refptr<CameraCapturer>& capturer) const;
    void logError(const webrtc::scoped_refptr<CameraCapturer>& capturer,
                  const std::string& message, int code = 0) const;
    void logVerbose(const webrtc::scoped_refptr<CameraCapturer>& capturer, const std::string& message) const;
private:
    const std::weak_ptr<CameraManager> _manager;
    SafeScopedRefPtr<CameraCapturer> _capturer;
};

} // namespace LiveKitCpp
