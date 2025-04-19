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
#pragma once // AsyncSharingSourceImpl.h
#include "AsyncVideoSourceImpl.h"

namespace LiveKitCpp
{

class DesktopCapturer;

class AsyncSharingSourceImpl : public AsyncVideoSourceImpl
{
public:
    AsyncSharingSourceImpl(std::unique_ptr<DesktopCapturer> capturer,
                           std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                           const std::shared_ptr<Bricks::Logger>& logger);
    ~AsyncSharingSourceImpl() final;
    // override of AsyncVideoSourceImpl
    void requestCapturer() final;
    void resetCapturer() final;
protected:
    // overrides of AsyncVideoSourceImpl
    void onOptionsChanged(const VideoOptions& options) final;
    MediaDeviceInfo validate(MediaDeviceInfo info) const final;
    VideoOptions validate(VideoOptions options) const final;
private:
    void startCapturer();
    void stopCapturer();
private:
    const std::unique_ptr<DesktopCapturer> _capturer;
};
	
} // namespace LiveKitCpp
