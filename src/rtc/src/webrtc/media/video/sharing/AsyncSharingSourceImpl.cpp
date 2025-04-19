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
#include "AsyncSharingSourceImpl.h"
#include "DesktopConfiguration.h"
#include "DesktopFrameVideoBuffer.h"
#include "VideoFrameImpl.h"

namespace LiveKitCpp
{

AsyncSharingSourceImpl::AsyncSharingSourceImpl(bool windowCapturer,
                                               std::unique_ptr<webrtc::DesktopCapturer> capturer,
                                               std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                               const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger)
    , _windowCapturer(windowCapturer)
    , _capturer(std::move(capturer))
    , _eventsTimer(timerQueue)
{
    if (_capturer) {
        _eventsTimer.setHighPrecision();
        _eventsTimer.setCallback(this);
    }
    setOptions(VideoOptions{._maxFPS = DesktopConfiguration::maxFramerate(_windowCapturer)});
}

AsyncSharingSourceImpl::~AsyncSharingSourceImpl()
{
    if (_capturer) {
        _eventsTimer.stop();
        _eventsTimer.setCallback(nullptr);
    }
    close();
}

void AsyncSharingSourceImpl::requestCapturer()
{
    if (_capturer && frameWanted()) {
        const auto captureOptions = this->options();
        _capturer->SetMaxFrameRate(captureOptions._maxFPS);
        // TODO: more safe version of starting required
        _eventsTimer.singleShot([this]() {
            _capturer->Start(this);
        });
        _eventsTimer.start(float(captureOptions._maxFPS));
    }
}

void AsyncSharingSourceImpl::resetCapturer()
{
    if (_capturer) {
        
    }
}

void AsyncSharingSourceImpl::OnFrameCaptureStart()
{
    notify(&MediaDeviceListener::onMediaStarted);
}

void AsyncSharingSourceImpl::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                             std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (webrtc::DesktopCapturer::Result::ERROR_PERMANENT == result) {
        changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
        // TODO: add error details
        notify(&MediaDeviceListener::onMediaFatalError, std::string{});
    }
    else if (frame) {
        auto buffer = webrtc::make_ref_counted<DesktopFrameVideoBuffer>(std::move(frame));
        if (const auto frame = createVideoFrame(buffer)) {
            broadcast(frame.value());
        }
    }
}

void AsyncSharingSourceImpl::onTimeout(uint64_t)
{
    if (_capturer) {
        _capturer->CaptureFrame();
    }
}

} // namespace LiveKitCpp
