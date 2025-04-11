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
#include "VideoFrameBuffer.h"
#include "livekit/rtc/media/CameraEventsListener.h"

namespace {

inline std::string makeCapturerError(int code, const std::string& what = {}) {
    std::string errorCode = "code #" + std::to_string(code);
    if (what.empty()) {
        return "capturer error - " + errorCode;
    }
    return what + ": " + errorCode;
}

inline bool isNull(const webrtc::VideoCaptureCapability& cap) {
    return 0 == cap.width || 0 == cap.height || 0 == cap.maxFPS;
}

}

namespace LiveKitCpp
{

AsyncCameraSourceImpl::AsyncCameraSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                             const std::shared_ptr<Bricks::Logger>& logger,
                                             const std::string& id,
                                             const MediaDeviceInfo& info,
                                             const webrtc::VideoCaptureCapability& initialCapability)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger, false)
    , _id(id)
    , _deviceInfo(info)
{
    if (isNull(initialCapability)) {
        _capability = CameraManager::defaultCapability();
    }
    else {
        _capability = initialCapability;
    }
    if (_deviceInfo->empty()) {
        CameraManager::defaultDevice(_deviceInfo.ref());
    }
}

void AsyncCameraSourceImpl::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (active()) {
        bool ok = true;
        MediaDeviceInfo deviceInfo(info);
        if (info._guid.empty()) {
            ok = CameraManager::defaultDevice(deviceInfo);
        }
        if (ok && !deviceInfo._guid.empty()) {
            bool changed = false;
            if (CameraManager::deviceIsValid(deviceInfo)) {
                LOCK_WRITE_SAFE_OBJ(_deviceInfo);
                if (_deviceInfo->_guid != deviceInfo._guid) {
                    _deviceInfo = std::move(deviceInfo);
                    changed = true;
                }
            }
            if (changed) {
                notify(&CameraEventsListener::onCapturerChanged, deviceInfo);
                resetCapturer();
                requestCapturer();
            }
        }
    }
}

void AsyncCameraSourceImpl::setCapability(webrtc::VideoCaptureCapability capability)
{
    if (active()) {
        if (isNull(capability)) {
            capability = CameraManager::defaultCapability();
        }
        LOCK_READ_SAFE_OBJ(_capturer);
        const auto& capturer = _capturer.constRef();
        if (capturer) {
            capability = bestMatched(std::move(capability), capturer);
        }
        bool changed = false;
        {
            LOCK_READ_SAFE_OBJ(_capability);
            if (capability != _capability.constRef()) {
                _capability = capability;
                changed = true;
            }
        }
        if (changed) {
            if (capturer && capturer->CaptureStarted()) {
                stopCapturer(false);
                startCapturer(capability);
            }
            notify(&CameraEventsListener::onOptionsChanged, map(capability));
        }
    }
}

void AsyncCameraSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer.constRef()) {
            const auto di = deviceInfo();
            if (auto capturer = CameraManager::createCapturer(di)) {
                capturer->RegisterCaptureDataCallback(this);
                capturer->setObserver(this);
                LOCK_WRITE_SAFE_OBJ(_capability);
                _capability = bestMatched(_capability.constRef(), capturer);
                _capturer = std::move(capturer);
                if (enabled()) {
                    startCapturer(_capability);
                }
            }
            else {
                notify(&CameraEventsListener::onCapturerCreationFailed, di);
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

void AsyncCameraSourceImpl::addListener(CameraEventsListener* listener)
{
    _listeners.add(listener);
}

void AsyncCameraSourceImpl::removeListener(CameraEventsListener* listener)
{
    _listeners.remove(listener);
}

std::string_view AsyncCameraSourceImpl::logCategory() const
{
    return CameraManager::logCategory();
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
    bestMatched(webrtc::VideoCaptureCapability capability, std::string_view guid)
{
    if (!guid.empty()) {
        webrtc::VideoCaptureCapability matched;
        if (CameraManager::bestMatchedCapability(guid, capability, matched)) {
            return matched;
        }
    }
    return capability;
}

webrtc::VideoCaptureCapability AsyncCameraSourceImpl::
    bestMatched(webrtc::VideoCaptureCapability capability,
                const rtc::scoped_refptr<CameraCapturer>& capturer)
{
    if (capturer) {
        return bestMatched(std::move(capability), capturer->guid());
    }
    return bestMatched(std::move(capability));
}

bool AsyncCameraSourceImpl::startCapturer(const webrtc::VideoCaptureCapability& capability)
{
    int32_t code = 0;
    if (frameWanted()) {
        if (const auto& capturer = _capturer.constRef()) {
            code = capturer->StartCapture(capability);
            if (0 != code) {
                logError(capturer, "failed to start capturer with caps [" + toString(capability) + "]", code);
                notify(&CameraEventsListener::onCapturingStartFailed, map(capability));
            }
            else {
                logVerbose(capturer, "capturer with caps [" + toString(capability) + "] has been started");
                notify(&CameraEventsListener::onCapturingStarted, map(capability));
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
            notify(&CameraEventsListener::onCapturingStopFailed);
        }
        else {
            logVerbose(capturer, "capturer has been stopped");
            notify(&CameraEventsListener::onCapturingStopped);
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

template <class Method, typename... Args>
void AsyncCameraSourceImpl::notify(const Method& method, Args&&... args) const
{
    _listeners.invoke(method, _id, std::forward<Args>(args)...);
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
    notify(&CameraEventsListener::onCapturingFatalError);
}

void AsyncCameraSourceImpl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        processConstraints(c);
    }
}

} // namespace LiveKitCpp
