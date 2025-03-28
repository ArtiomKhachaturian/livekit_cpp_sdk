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
#include "CameraVideoSourceImpl.h"
#include "CameraManager.h"
#include "VideoFrameBuffer.h"

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

CameraVideoSourceImpl::CameraVideoSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                             const std::shared_ptr<Bricks::Logger>& logger,
                                             const webrtc::VideoCaptureCapability& initialCapability)
    : VideoSourceImpl(std::move(signalingQueue), logger)
{
    if (isNull(initialCapability)) {
        _capability = CameraManager::defaultCapability();
    }
    else {
        _capability = initialCapability;
    }
    CameraManager::defaultDevice(_device.ref());
}

void CameraVideoSourceImpl::setDevice(MediaDevice device)
{
    if (active()) {
        bool ok = true;
        if (device._guid.empty()) {
            ok = CameraManager::defaultDevice(device);
        }
        if (ok && !device._guid.empty()) {
            bool changed = false;
            if (CameraManager::deviceIsValid(device)) {
                LOCK_WRITE_SAFE_OBJ(_device);
                if (_device->_guid != device._guid) {
                    _device = std::move(device);
                    changed = true;
                }
            }
            if (changed) {
                requestCapturer();
            }
        }
    }
}

void CameraVideoSourceImpl::setCapability(webrtc::VideoCaptureCapability capability)
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
        if (changed && capturer && capturer->CaptureStarted()) {
            stopCapturer(false);
            startCapturer(capability);
        }
    }
}

void CameraVideoSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer.constRef()) {
            if (auto capturer = CameraManager::createCapturer(device())) {
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
                // TODO: log error
            }
        }
    }
}

void CameraVideoSourceImpl::resetCapturer()
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    if (_capturer.constRef()) {
        stopCapturer(true);
        auto capturer = _capturer.take();
        capturer->DeRegisterCaptureDataCallback();
        capturer->setObserver(nullptr);
    }
}

std::string_view CameraVideoSourceImpl::logCategory() const
{
    return CameraManager::logCategory();
}

void CameraVideoSourceImpl::onClose()
{
    VideoSourceImpl::onClose();
    resetCapturer();
}

void CameraVideoSourceImpl::onEnabled(bool enabled)
{
    VideoSourceImpl::onEnabled(enabled);
    LOCK_READ_SAFE_OBJ(_capturer);
    if (_capturer.constRef()) {
        if (enabled) {
            startCapturer(_capability());
        }
        else {
            stopCapturer(true);
        }
    }
}

webrtc::VideoCaptureCapability CameraVideoSourceImpl::
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

webrtc::VideoCaptureCapability CameraVideoSourceImpl::
    bestMatched(webrtc::VideoCaptureCapability capability,
                const rtc::scoped_refptr<CameraCapturer>& capturer)
{
    if (capturer) {
        return bestMatched(std::move(capability), capturer->guid());
    }
    return bestMatched(std::move(capability));
}

bool CameraVideoSourceImpl::startCapturer(const webrtc::VideoCaptureCapability& capability)
{
    int32_t code = 0;
    if (frameWanted()) {
        if (const auto& capturer = _capturer.constRef()) {
            code = capturer->StartCapture(capability);
            if (0 != code) {
                logError(capturer, "failed to start capturer with caps [" + toString(capability) + "]", code);
            }
            else {
                logVerbose(capturer, "capturer with caps [" + toString(capability) + "] has been started");
            }
        }
    }
    return 0 == code;
}

bool CameraVideoSourceImpl::stopCapturer(bool sendByeFrame)
{
    int32_t code = 0;
    if (const auto& capturer = _capturer.constRef()) {
        uint16_t frameId = 0U;
        int w = 0, h = 0;
        if (sendByeFrame) {
            sendByeFrame = stats(w, h);
            if (sendByeFrame) {
                frameId = lastFrameId();
            }
        }
        code = capturer->StopCapture();
        if (0 != code) {
            logError(capturer, "failed to stop capturer",  code);
        }
        else {
            logVerbose(capturer, "capturer has been stopped");
        }
        if (sendByeFrame) {
            const auto frame = createBlackVideoFrame(w, h, 0, frameId + 1U);
            if (frame.has_value()) {
                broadcast(frame.value(), false);
            }
        }
    }
    return 0 == code;
}

void CameraVideoSourceImpl::logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                     const std::string& message, int code) const
{
    if (capturer && canLogError()) {
        const auto name = capturer->CurrentDeviceName();
        if (0 == code) {
            VideoSourceImpl::logError(CameraManager::formatLogMessage(name, message));
        }
        else {
            const auto error = makeCapturerError(code, message);
            VideoSourceImpl::logError(CameraManager::formatLogMessage(name, error));
        }
    }
}

void CameraVideoSourceImpl::logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                       const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        const auto error = CameraManager::formatLogMessage(capturer->CurrentDeviceName(), message);
        VideoSourceImpl::logVerbose(error);
    }
}

void CameraVideoSourceImpl::onStateChanged(CameraState state)
{
    switch (state) {
        case CameraState::Stopped:
            changeState(webrtc::MediaSourceInterface::SourceState::kEnded);
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

void CameraVideoSourceImpl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    if (active()) {
        processConstraints(c);
    }
}

} // namespace LiveKitCpp
