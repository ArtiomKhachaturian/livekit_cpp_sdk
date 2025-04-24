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
#include "SafeObjAliases.h"

namespace LiveKitCpp
{

class DesktopCapturer;
class DesktopConfiguration;

class AsyncSharingSourceImpl : public AsyncVideoSourceImpl
{
public:
    AsyncSharingSourceImpl(bool previewMode,
                           std::weak_ptr<DesktopConfiguration> desktopConfiguration,
                           std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                           const std::shared_ptr<Bricks::Logger>& logger);
    ~AsyncSharingSourceImpl() final { close(); }
    // override of AsyncVideoSourceImpl
    void requestCapturer() final;
    void resetCapturer() final;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
    // overrides of AsyncVideoSourceImpl
    void onOptionsChanged(const VideoOptions& options) final;
    void onContentHintChanged(VideoContentHint hint) final;
    void onDeviceInfoChanged(const MediaDeviceInfo& info) final;
    MediaDeviceInfo validate(MediaDeviceInfo info) const final;
private:
    void startCapturer(const std::unique_ptr<DesktopCapturer>& capturer) const;
    void stopCapturer(const std::unique_ptr<DesktopCapturer>& capturer) const;
    void logError(const std::unique_ptr<DesktopCapturer>& capturer, const std::string& message) const;
    void logVerbose(const std::unique_ptr<DesktopCapturer>& capturer, const std::string& message) const;
    static std::string formatLogMessage(const std::unique_ptr<DesktopCapturer>& capturer, const std::string& message);
    static std::string capturerTitle(const std::unique_ptr<DesktopCapturer>& capturer);
    static void applyOptions(DesktopCapturer* capturer, const VideoOptions& options);
private:
    const bool _previewMode;
    const std::weak_ptr<DesktopConfiguration> _desktopConfiguration;
    Bricks::SafeUniquePtr<DesktopCapturer> _capturer;
};
	
} // namespace LiveKitCpp
