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
#pragma once // MacDesktopCapturer.h
#include "DesktopCapturer.h"
#include <MacTypes.h>
#include <api/scoped_refptr.h>
#include <CoreGraphics/CGDirectDisplay.h>
#include <dispatch/dispatch.h>
#include <optional>

namespace webrtc {
class DesktopCaptureOptions;
class DesktopConfigurationMonitor;
class TaskQueueBase;
} // namespace webrtc

namespace LiveKitCpp
{

class MacDesktopCapturer : public DesktopCapturer
{
public:
    static bool isWindowServerWindow(webrtc::WindowId wId);
    static bool screenExists(webrtc::ScreenId sId);
    static bool windowExists(webrtc::WindowId wId);
    static std::string screenTitle(webrtc::ScreenId sId);
    static std::optional<std::string> windowTitle(webrtc::WindowId wId);
    static webrtc::DesktopSize screenResolution(const webrtc::scoped_refptr<webrtc::DesktopConfigurationMonitor>& monitor,
                                                webrtc::ScreenId sId);
    static std::unique_ptr<DesktopCapturer> create(bool window, const webrtc::DesktopCaptureOptions& options,
                                                   const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue);
    const auto& monitor() const { return _monitor; }
    ~MacDesktopCapturer() override;
    // overrides of DesktopCapturer
    std::optional<std::string> title(const std::string& source) const override;
    webrtc::DesktopSize screenResolution(const std::string& source) const override;
    bool enumerateSources(std::vector<std::string>& sources) const override;
    void setSharedMemoryFactory(std::unique_ptr<webrtc::SharedMemoryFactory> smf) final;
protected:
    MacDesktopCapturer(bool window, const webrtc::scoped_refptr<webrtc::DesktopConfigurationMonitor>& monitor);
    const std::shared_ptr<webrtc::SharedMemoryFactory>& sharedMemoryFactory() const { return _smf; }
    std::unique_ptr<webrtc::DesktopFrame> captureDisplay(webrtc::ScreenId sId) const;
    std::unique_ptr<webrtc::DesktopFrame> captureWindow(webrtc::WindowId wId) const;
    // the same value as in WebKit, system default is 3, max should not exceed 8 frames
    static constexpr int screenQueueMaximumLength() { return 6; }
    static OSType recommendedVideoFormat();
    static dispatch_queue_t currentQueue();
private:
    // monitoring display reconfiguration
    const webrtc::scoped_refptr<webrtc::DesktopConfigurationMonitor> _monitor;
    // shared memory factory
    std::shared_ptr<webrtc::SharedMemoryFactory> _smf;
};


} // namespace LiveKitCpp
