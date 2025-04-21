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
#include "CGScreenCapturer.h"
#include "DesktopCapturerUtils.h"
#include "Utils.h"

namespace {

inline constexpr bool validScreenId(webrtc::ScreenId id) {
    return id != webrtc::kInvalidScreenId;
}

}

namespace LiveKitCpp
{

CGScreenCapturer::CGScreenCapturer(webrtc::DesktopCaptureOptions options)
    : Base(false, std::move(options))
{
    allocateCursorComposer();
}

CGScreenCapturer::CGScreenCapturer(webrtc::DesktopCaptureOptions options,
                                   std::shared_ptr<webrtc::TaskQueueBase> timerQueue)
    : Base(false, std::move(options), std::move(timerQueue))
{
    allocateCursorComposer();
}

CGScreenCapturer::~CGScreenCapturer()
{
    stop();
    destroyCursorComposer();
}

void CGScreenCapturer::setPreviewMode(bool preview)
{
    if (preview) {
        destroyCursorComposer();
    }
    else {
        allocateCursorComposer();
    }
}

bool CGScreenCapturer::selectSource(const std::string& source)
{
    if (!started()) {
        const auto id = screenIdFromString(source);
        if (id.has_value()) {
            _source = id.value();
            return validScreenId(id.value());
        }
    }
    return false;
}

void CGScreenCapturer::captureNextFrame()
{
    deliverCaptured(processFrame(captureDisplay(static_cast<webrtc::ScreenId>(_source))));
}

bool CGScreenCapturer::canStart() const
{
    return Base::canStart() && validScreenId(_source);
}

void CGScreenCapturer::allocateCursorComposer()
{
    webrtc::DesktopCapturer::Callback* callback = this;
    std::atomic_store(&_cursorComposer, std::make_shared<DesktopWebRTCCursorComposer>(options(), callback));
}

void CGScreenCapturer::destroyCursorComposer()
{
    if (auto composer = std::atomic_exchange(&_cursorComposer, std::shared_ptr<DesktopWebRTCCursorComposer>{})) {
        composer->setCallback(nullptr);
    }
}

webrtc::DesktopVector CGScreenCapturer::dpi() const
{
    auto dpiVal = webrtc::kStandardDPI;
    if (const auto& monitor = options().configuration_monitor()) {
        auto config = monitor->desktop_configuration();
        float dipToPixelScale = 1.;
        const auto displayId = static_cast<CGDirectDisplayID>(_source);
        if (const auto screenConfig = config.FindDisplayConfigurationById(displayId)) {
            dipToPixelScale = screenConfig->dip_to_pixel_scale;
        }
        else {
            dipToPixelScale = config.dip_to_pixel_scale;
        }
        dpiVal *= std::max<float>(1., dipToPixelScale);
    }
    return webrtc::DesktopVector(dpiVal, dpiVal);
}

std::unique_ptr<webrtc::DesktopFrame> CGScreenCapturer::processFrame(std::unique_ptr<webrtc::DesktopFrame> frame) const
{
    if (frame) {
        frame->set_dpi(dpi());
        if (!frame->may_contain_cursor()) {
            if (const auto composer = std::atomic_load(&_cursorComposer)) {
                composer->setFrame(std::move(frame));
            }
        }
    }
    return frame;
}

void CGScreenCapturer::OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                                        std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (frame) {
        deliverCaptured(std::move(frame));
    }
    else {
        notifyAboutError("capture has failed",webrtc::DesktopAndCursorComposer::Result::ERROR_PERMANENT == result);
    }
}

} // namespace LiveKitCpp
