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
#include "MacDesktopCapturer.h"
#include "CGScreenCapturer.h"
#include "VTSupportedPixelFormats.h"
#include <modules/desktop_capture/mac/desktop_frame_cgimage.h>

namespace LiveKitCpp
{

MacDesktopCapturer::MacDesktopCapturer(bool window, const webrtc::DesktopCaptureOptions& options)
    : DesktopCapturer(window, options)
{
}

MacDesktopCapturer::~MacDesktopCapturer()
{
}

std::unique_ptr<DesktopCapturer> MacDesktopCapturer::create(bool window,
                                                            const webrtc::DesktopCaptureOptions& options,
                                                            const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue)
{
    if (!window) {
        if (@available(macOS 12.3, *)) {
            // ScreenCaptureKitCapturer
        }
        return std::make_unique<CGScreenCapturer>(timerQueue, options);
    }
    return {};
}

std::unique_ptr<webrtc::DesktopFrame> MacDesktopCapturer::captureDisplay(webrtc::ScreenId sId) const
{
    const auto id = static_cast<CGDirectDisplayID>(sId);
    return webrtc::DesktopFrameCGImage::CreateForDisplay(id);
}

std::unique_ptr<webrtc::DesktopFrame> MacDesktopCapturer::captureWindow(webrtc::WindowId wId) const
{
    const auto id = static_cast<CGWindowID>(wId);
    return webrtc::DesktopFrameCGImage::CreateForWindow(id);
}

OSType MacDesktopCapturer::recommendedVideoFormat()
{
    return pixelFormatNV12Video();
}

dispatch_queue_t MacDesktopCapturer::currentQueue()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    return dispatch_get_current_queue();
#pragma GCC diagnostic pop
}

} // namespace LiveKitCpp
