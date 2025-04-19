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
#include "AsyncCameraSourceImpl.h"
#include "CameraManager.h"
#include "VideoUtils.h"

namespace {

inline std::string makeCapturerError(int code, const std::string& what = {}) {
    std::string errorCode = "code #" + std::to_string(code);
    if (what.empty()) {
        return "capturer error - " + errorCode;
    }
    return what + ": " + errorCode;
}

}

namespace LiveKitCpp
{

AsyncCameraSourceImpl::AsyncCameraSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                             const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger, false)
{
    setOptions(map(CameraManager::defaultCapability()));
}

void AsyncCameraSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer.constRef()) {
            if (auto capturer = CameraManager::createCapturer(deviceInfo())) {
                const auto capability = bestMatched(map(options()), capturer);
                capturer->setContentHint(contentHint());
                capturer->RegisterCaptureDataCallback(this);
                capturer->setObserver(this);
                _capturer = std::move(capturer);
                if (enabled()) {
                    startCapturer(capability);
                }
            }
            else {
                // TODO: add error details
                notify(&MediaDeviceListener::onMediaCreationFailed, std::string{});
            }
        }
    }
}

void AsyncCameraSourceImpl::resetCapturer()
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    if (_capturer.constRef()) {
        stopCapturer(true);
        auto capturer = _capturer.take();
        capturer->DeRegisterCaptureDataCallback();
        capturer->setObserver(nullptr);
    }
}

std::string_view AsyncCameraSourceImpl::logCategory() const
{
    return CameraManager::logCategory();
}

void AsyncCameraSourceImpl::onContentHintChanged(VideoContentHint hint)
{
    AsyncVideoSourceImpl::onContentHintChanged(hint);
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        capturer->setContentHint(hint);
    }
}

void AsyncCameraSourceImpl::onOptionsChanged(const VideoOptions& options)
{
    AsyncVideoSourceImpl::onOptionsChanged(options);
    if (active()) {
        LOCK_READ_SAFE_OBJ(_capturer);
        const auto& capturer = _capturer.constRef();
        if (capturer && capturer->CaptureStarted()) {
            stopCapturer(false);
            startCapturer(bestMatched(map(options), capturer));
        }
    }
}

MediaDeviceInfo AsyncCameraSourceImpl::validate(MediaDeviceInfo info) const
{
    bool ok = true;
    if (info._guid.empty()) {
        ok = CameraManager::defaultDevice(info);
    }
    if (ok) {
        return info;
    }
    return {};
}

VideoOptions AsyncCameraSourceImpl::validate(VideoOptions options) const
{
    webrtc::VideoCaptureCapability capability;
    if (!options) {
        capability = CameraManager::defaultCapability();
    }
    else {
        capability = map(options);
    }
    return map(bestMatched(std::move(capability)));
}

void AsyncCameraSourceImpl::onClosed()
{
    AsyncVideoSourceImpl::onClosed();
    resetCapturer();
}

void AsyncCameraSourceImpl::onEnabled(bool enabled)
{
    AsyncVideoSourceImpl::onEnabled(enabled);
    if (enabled) {
        requestCapturer();
    }
    else {
        resetCapturer();
    }
}

webrtc::VideoCaptureCapability AsyncCameraSourceImpl::
    bestMatched(webrtc::VideoCaptureCapability capability,
                const rtc::scoped_refptr<CameraCapturer>& capturer)
{
    if (capturer) {
        webrtc::VideoCaptureCapability matched;
        if (CameraManager::bestMatchedCapability(capturer->guid(), capability, matched)) {
            return matched;
        }
    }
    return capability;
}

webrtc::VideoCaptureCapability AsyncCameraSourceImpl::bestMatched(webrtc::VideoCaptureCapability capability) const
{
    return bestMatched(std::move(capability), _capturer());
}

bool AsyncCameraSourceImpl::startCapturer(const webrtc::VideoCaptureCapability& capability)
{
    int32_t code = 0;
    if (enabled() && active() && frameWanted()) {
        if (const auto& capturer = _capturer.constRef()) {
            code = capturer->StartCapture(capability);
            if (0 != code) {
                logError(capturer, "failed to start capturer with caps [" + toString(capability) + "]", code);
                // TODO: add error details
                notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
            }
            else {
                logVerbose(capturer, "capturer with caps [" + toString(capability) + "] has been started");
                notify(&MediaDeviceListener::onMediaStarted);
            }
        }
    }
    return 0 == code;
}

bool AsyncCameraSourceImpl::stopCapturer(bool sendByeFrame)
{
    int32_t code = 0;
    const auto& capturer = _capturer.constRef();
    if (capturer && capturer->CaptureStarted()) {
        uint16_t frameId = 0U;
        int w = 0, h = 0;
        if (sendByeFrame) {
            sendByeFrame = stats(w, h);
            if (sendByeFrame) {
                frameId = lastFrameId();
            }
        }
        code = capturer->StopCapture();
        if (sendByeFrame) {
            const auto frame = createBlackVideoFrame(w, h, 0, frameId + 1U);
            if (frame.has_value()) {
                broadcast(frame.value(), false);
            }
        }
        if (0 != code) {
            logError(capturer, "failed to stop capturer",  code);
            // TODO: add error details
            notify(&MediaDeviceListener::onMediaStopFailed, std::string{});
        }
        else {
            logVerbose(capturer, "capturer has been stopped");
            notify(&MediaDeviceListener::onMediaStopped);
        }
    }
    return 0 == code;
}

void AsyncCameraSourceImpl::logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                     const std::string& message, int code) const
{
    if (capturer && canLogError()) {
        const auto name = capturer->CurrentDeviceName();
        if (0 == code) {
            AsyncVideoSourceImpl::logError(CameraManager::formatLogMessage(name, message));
        }
        else {
            const auto error = makeCapturerError(code, message);
            AsyncVideoSourceImpl::logError(CameraManager::formatLogMessage(name, error));
        }
    }
}

void AsyncCameraSourceImpl::logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                       const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        const auto error = CameraManager::formatLogMessage(capturer->CurrentDeviceName(), message);
        AsyncVideoSourceImpl::logVerbose(error);
    }
}

void AsyncCameraSourceImpl::onStateChanged(CameraState state)
{
    switch (state) {
        case CameraState::Stopped:
            changeState(webrtc::MediaSourceInterface::SourceState::kMuted);
            break;
        case CameraState::Starting:
            changeState(webrtc::MediaSourceInterface::SourceState::kInitializing);
            break;
        case CameraState::Started:
            changeState(webrtc::MediaSourceInterface::SourceState::kLive);
            break;
        default:
            break;
    }
}

void AsyncCameraSourceImpl::onCapturingFatalError()
{
    changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
    // TODO: add error details
    notify(&MediaDeviceListener::onMediaFatalError, std::string{});
}

void AsyncCameraSourceImpl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        processConstraints(c);
    }
}

} // namespace LiveKitCpp
