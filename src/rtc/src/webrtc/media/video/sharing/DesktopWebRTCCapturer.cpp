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
#include "DesktopWebRTCCapturer.h"
#include "DesktopWebRTCCapturerImpl.h"
#include "ThreadUtils.h"

namespace LiveKitCpp
{

DesktopWebRTCCapturer::DesktopWebRTCCapturer(bool window, bool previewMode,
                                             webrtc::DesktopCaptureOptions options,
                                             std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                                             VideoFrameBufferPool framesPool)
    : DesktopSimpleCapturer<DesktopCapturer>(window, previewMode, std::move(options),
                                             std::move(timerQueue), std::move(framesPool))
    , RtcObject<DesktopWebRTCCapturerImpl, webrtc::DesktopCapturer::Callback>(window)
{
    if (const auto impl = loadImpl()) {
        impl->setCallback(this);
    }
}

DesktopWebRTCCapturer::DesktopWebRTCCapturer(bool window, bool previewMode,
                                             webrtc::DesktopCaptureOptions options,
                                             VideoFrameBufferPool framesPool)
    : DesktopSimpleCapturer<DesktopCapturer>(window, previewMode,
                                             std::move(options), std::move(framesPool))
    , RtcObject<DesktopWebRTCCapturerImpl, webrtc::DesktopCapturer::Callback>(window)
{
}

DesktopWebRTCCapturer::~DesktopWebRTCCapturer()
{
    if (auto impl = dispose()) {
        impl->setCallback(nullptr);
        execute([impl = std::move(impl)]() mutable {
            impl->stop();
            impl.reset();
        });
        changeState(CapturerState::Stopped);
    }
}

bool DesktopWebRTCCapturer::selectSource(const std::string& source)
{
    if (!started()) {
        const auto sourceId = parse(source);
        if (sourceId) {
            _selectedSource(source);
            postToImpl(&DesktopWebRTCCapturerImpl::selectSource, sourceId.value());
        }
        return hasValidSource();
    }
    return false;
}

bool DesktopWebRTCCapturer::start()
{
    if (DesktopSimpleCapturer<DesktopCapturer>::start()) {
        changeState(CapturerState::Starting);
        postToImpl(&DesktopWebRTCCapturerImpl::start, options());
        return true;
    }
    return false;
}

void DesktopWebRTCCapturer::stop()
{
    if (started()) {
        changeState(CapturerState::Stopping);
        postToImpl(&DesktopWebRTCCapturerImpl::stop);
        DesktopSimpleCapturer<DesktopCapturer>::stop();
        changeState(CapturerState::Stopped);
    }
}

void DesktopWebRTCCapturer::focusOnSelectedSource()
{
    if (!previewMode()) {
        postToImpl(&DesktopWebRTCCapturerImpl::focusOnSelectedSource);
    }
    DesktopSimpleCapturer<DesktopCapturer>::focusOnSelectedSource();
}

void DesktopWebRTCCapturer::setExcludedWindow(webrtc::WindowId window)
{
    postToImpl(&DesktopWebRTCCapturerImpl::setExcludedWindow, window);
    DesktopSimpleCapturer<DesktopCapturer>::setExcludedWindow(window);
}

void DesktopWebRTCCapturer::captureNextFrame()
{
    postToImpl(&DesktopWebRTCCapturerImpl::captureFrame);
}

bool DesktopWebRTCCapturer::canStart() const
{
    return DesktopSimpleCapturer<DesktopCapturer>::canStart() && hasValidSource();
}

std::optional<intptr_t> DesktopWebRTCCapturer::parse(const std::string& source) const
{
    if (window()) {
        return windowIdFromString(source);
    }
    return screenIdFromString(source);
}

bool DesktopWebRTCCapturer::hasValidSource() const
{
    LOCK_READ_SAFE_OBJ(_selectedSource);
    return !_selectedSource->empty();
}

template <class Method, typename... Args>
void DesktopWebRTCCapturer::postToImpl(Method method, Args&&... args) const
{
    if (const auto impl = loadImpl()) {
        postOrInvokeS<true>(timerQueue(), impl, method, std::forward<Args>(args)...);
    }
}

void DesktopWebRTCCapturer::OnFrameCaptureStart()
{
    changeState(CapturerState::Started);
}

void DesktopWebRTCCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                            std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (frame) {
        changeState(CapturerState::Started);
        deliverCaptured(std::move(frame));
    }
    else {
        if (webrtc::DesktopCapturer::Result::ERROR_PERMANENT == result) {
            changeState(CapturerState::Stopped);
        }
        notifyAboutError("capture has failed", webrtc::DesktopCapturer::Result::ERROR_PERMANENT == result);
    }
}

} // namespace LiveKitCpp
