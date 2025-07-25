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
                                             std::weak_ptr<CameraManager> manager,
                                             const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSourceImpl(std::move(signalingQueue), logger, defaultCameraContentHint())
    , _manager(std::move(manager))
{
    setOptions(map(CameraManager::defaultCapability()));
}

bool AsyncCameraSourceImpl::changeContentHint(VideoContentHint hint)
{
    switch (hint) {
        case VideoContentHint::None:
        case VideoContentHint::Motion:
            return AsyncVideoSourceImpl::changeContentHint(hint);
        default:
            break;
    }
    return false;
}

void AsyncCameraSourceImpl::requestCapturer()
{
    if (frameWanted()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer.constRef()) {
            if (auto capturer = create(deviceInfo())) {
                const auto capability = bestMatched(map(options()), capturer);
                capturer->RegisterCaptureDataCallback(this);
                capturer->setObserver(this);
                _capturer = std::move(capturer);
                if (enabled()) {
                    startCapturer(_capturer.constRef(), capability);
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
    if (auto capturer = _capturer.take()) {
        stopCapturer(capturer);
        capturer->DeRegisterCaptureDataCallback();
        capturer->setObserver(nullptr);
    }
}

void AsyncCameraSourceImpl::updateAfterContentHintChanges(VideoContentHint hint)
{
    AsyncVideoSourceImpl::updateAfterContentHintChanges(hint);
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        capturer->updateQualityToContentHint();
    }
}

std::string_view AsyncCameraSourceImpl::logCategory() const
{
    return CameraManager::logCategory();
}

void AsyncCameraSourceImpl::onCapturingError(std::string details, bool fatal)
{
    if (canLogError()) {
        logError(_capturer(), details);
    }
    AsyncVideoSourceImpl::onCapturingError(std::move(details), fatal);
}

void AsyncCameraSourceImpl::onOptionsChanged(const VideoOptions& options)
{
    AsyncVideoSourceImpl::onOptionsChanged(options);
    if (active()) {
        LOCK_READ_SAFE_OBJ(_capturer);
        const auto& capturer = _capturer.constRef();
        if (capturer && capturer->CaptureStarted()) {
            stopCapturer(capturer);
            startCapturer(capturer, bestMatched(map(options), capturer));
        }
    }
}

MediaDeviceInfo AsyncCameraSourceImpl::validate(MediaDeviceInfo info) const
{
    bool ok = true;
    if (info._guid.empty()) {
        if (const auto manager = _manager.lock()) {
            ok = manager->defaultDevice(info);
        }
        else {
            ok = false;
        }
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

webrtc::VideoCaptureCapability AsyncCameraSourceImpl::
    bestMatched(webrtc::VideoCaptureCapability capability,
                const webrtc::scoped_refptr<CameraCapturer>& capturer) const
{
    if (capturer) {
        webrtc::VideoCaptureCapability matched;
        const auto manager = _manager.lock();
        if (manager && manager->bestMatchedCapability(capturer->guid(), capability, matched)) {
            return matched;
        }
    }
    return capability;
}

webrtc::VideoCaptureCapability AsyncCameraSourceImpl::bestMatched(webrtc::VideoCaptureCapability capability) const
{
    return bestMatched(std::move(capability), _capturer());
}

webrtc::scoped_refptr<CameraCapturer> AsyncCameraSourceImpl::create(const MediaDeviceInfo& dev) const
{
    if (const auto manager = _manager.lock()) {
        return manager->createCapturer(dev, framesPool(), logger());
    }
    return {};
}

bool AsyncCameraSourceImpl::startCapturer(const webrtc::scoped_refptr<CameraCapturer>& capturer,
                                          const webrtc::VideoCaptureCapability& capability) const
{
    int32_t code = 0;
    if (capturer && enabled() && active() && frameWanted()) {
        code = capturer->StartCapture(capability);
        if (0 != code) {
            if (canLogError()) {
                logError(capturer, "failed to start capturer with caps [" + toString(capability) + "]", code);
            }
            // TODO: add error details
            notify(&MediaDeviceListener::onMediaStartFailed, std::string{});
        }
        else {
            if (canLogVerbose()) {
                logVerbose(capturer, "capturer with caps [" + toString(capability) + "] has been started");
            }
            notify(&MediaDeviceListener::onMediaStarted);
        }
    }
    return 0 == code;
}

bool AsyncCameraSourceImpl::stopCapturer(const webrtc::scoped_refptr<CameraCapturer>& capturer) const
{
    int32_t code = 0;
    if (capturer && capturer->CaptureStarted()) {
        code = capturer->StopCapture();
        if (0 != code) {
            if (canLogError()) {
                logError(capturer, "failed to stop capturer",  code);
            }
            // TODO: add error details
            notify(&MediaDeviceListener::onMediaStopFailed, std::string{});
        }
        else {
            if (canLogVerbose()) {
                logVerbose(capturer, "capturer has been stopped");
            }
            notify(&MediaDeviceListener::onMediaStopped);
        }
    }
    return 0 == code;
}

void AsyncCameraSourceImpl::logError(const webrtc::scoped_refptr<CameraCapturer>& capturer,
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

void AsyncCameraSourceImpl::logVerbose(const webrtc::scoped_refptr<CameraCapturer>& capturer,
                                       const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        const auto verbose = CameraManager::formatLogMessage(capturer->CurrentDeviceName(), message);
        AsyncVideoSourceImpl::logVerbose(verbose);
    }
}

} // namespace LiveKitCpp
