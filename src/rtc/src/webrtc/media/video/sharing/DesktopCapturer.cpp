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
#include "DesktopCapturer.h"
#include "DesktopFrameVideoBuffer.h"
#include "DesktopCapturerUtils.h"
#include "CapturerProxySink.h"
#include "VideoUtils.h"

namespace LiveKitCpp
{

DesktopCapturer::DesktopCapturer(bool window, webrtc::DesktopCaptureOptions options)
    : _window(window)
    , _options(std::move(options))
{
}

std::string DesktopCapturer::windowIdToString(webrtc::WindowId id) const
{
    return LiveKitCpp::windowIdToString(id);
}

std::string DesktopCapturer::screenIdToString(webrtc::ScreenId id) const
{
    return LiveKitCpp::screenIdToString(id);
}

std::optional<webrtc::WindowId> DesktopCapturer::windowIdFromString(const std::string& str) const
{
    return LiveKitCpp::windowIdFromString(str);
}

std::optional<webrtc::ScreenId> DesktopCapturer::screenIdFromString(const std::string& str) const
{
    return LiveKitCpp::screenIdFromString(str);
}

std::optional<std::string> DesktopCapturer::title(const std::string& source) const
{
    if (!source.empty()) {
        if (window()) {
            if (const auto id = this->windowIdFromString(source)) {
                return windowTitle(id.value());
            }
        }
        if (const auto id = this->screenIdFromString(source)) {
            return screenTitle(id.value());
        }
    }
    return std::nullopt;
}

webrtc::DesktopSize DesktopCapturer::screenResolution(const std::string& source) const
{
    if (!window()) {
        if (const auto id = this->screenIdFromString(source)) {
            return LiveKitCpp::screenResolution(options(), id.value());
        }
    }
    return {};
}

bool DesktopCapturer::enumerateSources(std::vector<std::string>& sources) const
{
    if (window()) {
        return enumerateWindows(options(), sources);
    }
    return enumerateScreens(options(), sources);
}

void DesktopCapturer::setTargetResolution(const webrtc::DesktopSize& resolution)
{
    setTargetResolution(resolution.width(), resolution.height());
}

void DesktopCapturer::setOutputSink(CapturerProxySink* sink)
{
    _sink = sink;
}

bool DesktopCapturer::changeState(CapturerState state)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_state);
        if (_state.constRef() != state) {
            changed = acceptState(_state.constRef(), state);
            if (changed) {
                _state = state;
            }
        }
    }
    if (changed) {
        _sink.invoke(&CapturerProxySink::onStateChanged, state);
    }
    return changed;
}

void DesktopCapturer::notifyAboutError(std::string details, bool fatal) const
{
    _sink.invoke(&CapturerProxySink::onCapturingError, std::move(details), fatal);
}

void DesktopCapturer::discardFrame()
{
    _sink.invoke(&CapturerProxySink::OnDiscardedFrame);
}

void DesktopCapturer::deliverCaptured(const webrtc::VideoFrame& frame)
{
    if (frame.width() && frame.height()) {
        _lastFrameId = frame.id();
        _sink.invoke(&CapturerProxySink::OnFrame, frame);
    }
}

void DesktopCapturer::deliverCaptured(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                      int64_t timeStampMicro,
                                      webrtc::VideoRotation rotation,
                                      const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (auto frame = createVideoFrame(buff, timeStampMicro, _lastFrameId + 1U, colorSpace)) {
        frame->set_rotation(rotation);
        deliverCaptured(frame.value());
    }
}

void DesktopCapturer::deliverCaptured(std::unique_ptr<webrtc::DesktopFrame> frame,
                                      const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (frame) {
        const auto timestamp = frame->capture_time_ms();
        const auto buffer = webrtc::make_ref_counted<DesktopFrameVideoBuffer>(std::move(frame), framesPool());
        deliverCaptured(buffer, timestamp * 1000);
    }
}

void DesktopCapturer::processConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    _sink.invoke(&CapturerProxySink::OnConstraintsChanged, c);
}

} // namespace LiveKitCpp
