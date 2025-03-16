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
#include "CameraVideoSource.h"
#include "CameraManager.h"
#include "CameraCapturer.h"
#include "VideoSinkBroadcast.h"
#include "Utils.h"

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

CameraVideoSource::CameraVideoSource(std::weak_ptr<rtc::Thread> signalingThread,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
    , _observers(std::move(signalingThread))
    , _capability(CameraManager::defaultCapability())
    , _state(webrtc::MediaSourceInterface::kEnded)
{
    CameraManager::defaultDevice(_device.ref());
}

CameraVideoSource::~CameraVideoSource()
{
    resetCapturer();
    _broadcasters({});
}

void CameraVideoSource::setDevice(MediaDevice device)
{
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

void CameraVideoSource::setCapability(webrtc::VideoCaptureCapability capability)
{
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
        stopCapturer(capturer);
        startCapturer(capturer, capability);
    }
}

bool CameraVideoSource::setEnabled(bool enabled)
{
    if (enabled != _enabled.exchange(enabled)) {
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            it->second->applyBlackFrames(!enabled);
        }
        return true;
    }
    return false;
}

bool CameraVideoSource::GetStats(Stats* stats)
{
    if (stats) {
        const auto lastResolution = _lastResolution.load();
        if (lastResolution) {
            stats->input_width = extractHiWord(lastResolution);
            stats->input_height = extractLoWord(lastResolution);
            return true;
        }
    }
    return false;
}

void CameraVideoSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnConstraintsChanged(c);
    }
}

void CameraVideoSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                        const rtc::VideoSinkWants& wants)
{
    if (sink) {
        bool maybeStart = false;
        {
            LOCK_WRITE_SAFE_OBJ(_broadcasters);
            const auto it = _broadcasters->find(sink);
            if (it != _broadcasters->end()) {
                it->second->updateSinkWants(wants);
            }
            else {
                auto adapter = std::make_unique<VideoSinkBroadcast>(sink, wants);
                _broadcasters->insert(std::make_pair(sink, std::move(adapter)));
                maybeStart = 1U == _broadcasters->size();
            }
        }
        if (maybeStart) {
            requestCapturer();
        }
    }
}

void CameraVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        bool maybeStop = false;
        {
            LOCK_WRITE_SAFE_OBJ(_broadcasters);
            if (_broadcasters->erase(sink) > 0U) {
                maybeStop = _broadcasters->empty();
            }
        }
        if (maybeStop) {
            resetCapturer();
        }
    }
}

void CameraVideoSource::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.add(observer);
}

void CameraVideoSource::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.remove(observer);
}

void CameraVideoSource::onStateChanged(CameraState state)
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

void CameraVideoSource::OnFrame(const webrtc::VideoFrame& frame)
{
    if (frame.video_frame_buffer()) {
        _lastResolution = clueToUint64(frame.width(), frame.height());
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            it->second->OnFrame(frame);
        }
    }
}

void CameraVideoSource::OnDiscardedFrame()
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnDiscardedFrame();
    }
}

void CameraVideoSource::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints)
{
    ProcessConstraints(constraints);
}

webrtc::VideoCaptureCapability CameraVideoSource::bestMatched(webrtc::VideoCaptureCapability capability,
                                                              std::string_view guid)
{
    if (!guid.empty()) {
        webrtc::VideoCaptureCapability matched;
        if (CameraManager::bestMatchedCapability(guid, capability, matched)) {
            return matched;
        }
    }
    return capability;
}

webrtc::VideoCaptureCapability CameraVideoSource::bestMatched(webrtc::VideoCaptureCapability capability,
                                                              const rtc::scoped_refptr<CameraCapturer>& capturer)
{
    if (capturer) {
        return bestMatched(std::move(capability), capturer->guid());
    }
    return bestMatched(std::move(capability));
}

bool CameraVideoSource::startCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                      const webrtc::VideoCaptureCapability& capability) const
{
    if (capturer) {
        const auto code = capturer->StartCapture(capability);
        if (0 != code) {
            logError(capturer, "failed to start capturer with caps [" + toString(capability) + "]", code);
        }
        else {
            logVerbose(capturer, "capturer with caps [" + toString(capability) + "] has been started");
        }
        return 0 == code;
    }
    return false;
}

bool CameraVideoSource::stopCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer) const
{
    if (capturer) {
        const auto code = capturer->StopCapture();
        if (0 != code) {
            logError(capturer, "failed to stop capturer",  code);
        }
        else {
            logVerbose(capturer, "capturer has been stopped");
        }
        return 0 == code;
    }
    return false;
}

void CameraVideoSource::logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                 const std::string& message, int code) const
{
    if (capturer && canLogError()) {
        const auto name = capturer->CurrentDeviceName();
        if (0 == code) {
            Base::logError(CameraManager::formatLogMessage(name, message));
        }
        else {
            Base::logError(CameraManager::formatLogMessage(name, makeCapturerError(code, message)));
        }
    }
}

void CameraVideoSource::logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer, const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        Base::logVerbose(CameraManager::formatLogMessage(capturer->CurrentDeviceName(), message));
    }
}

void CameraVideoSource::requestCapturer()
{
    if (frameWanted()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer.constRef()) {
            LOCK_READ_SAFE_OBJ(_device);
            if (const auto capturer = CameraManager::createCapturer(_device.constRef())) {
                LOCK_WRITE_SAFE_OBJ(_capability);
                _capability = bestMatched(_capability.constRef(), capturer);
                destroyCapturer();
                capturer->RegisterCaptureDataCallback(this);
                capturer->setObserver(this);
                _capturer = capturer;
                startCapturer(capturer, _capability.constRef());
            }
            else {
                // TODO: log error
            }
        }
    }
}

void CameraVideoSource::resetCapturer()
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    destroyCapturer();
}

void CameraVideoSource::destroyCapturer()
{
    if (auto capturer = _capturer.take()) {
        stopCapturer(capturer);
        capturer->DeRegisterCaptureDataCallback();
        capturer->setObserver(nullptr);
    }
}

bool CameraVideoSource::frameWanted() const
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    return !_broadcasters->empty();
}

void CameraVideoSource::changeState(webrtc::MediaSourceInterface::SourceState state)
{
    if (state != _state.exchange(state)) {
        switch (state) {
            case webrtc::MediaSourceInterface::SourceState::kEnded:
                _lastResolution = 0ULL;
                break;
            default:
                break;
        }
        _observers.invoke(&webrtc::ObserverInterface::OnChanged);
    }
}

std::string_view CameraVideoSource::logCategory() const
{
    return CameraManager::logCategory();
}

} // namespace LiveKitCpp
