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
    : Bricks::LoggableS<CameraCapturerProxySink>(logger)
    , _observers(std::move(signalingThread))
    , _capability(CameraManager::defaultCapability())
    , _state(webrtc::MediaSourceInterface::kEnded)
{
}

CameraVideoSource::~CameraVideoSource()
{
    setCapturer({});
    LOCK_WRITE_SAFE_OBJ(_broadcasters);
    _broadcasters->clear();
}

void CameraVideoSource::setCapturer(rtc::scoped_refptr<CameraCapturer> capturer)
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    if (capturer != _capturer.constRef()) {
        if (const auto& prev = _capturer.constRef()) {
            stop(prev);
            prev->DeRegisterCaptureDataCallback();
            prev->setObserver(nullptr);
        }
        _capturer = std::move(capturer);
        if (const auto& capturer = _capturer.constRef()) {
            capturer->RegisterCaptureDataCallback(this);
            capturer->setObserver(this);
            LOCK_WRITE_SAFE_OBJ(_capability);
            _capability = bestMatched(_capability.constRef(), _capturer.constRef());
            if (frameWanted()) {
                start(capturer, _capability.constRef());
            }
        }
    }
}

void CameraVideoSource::setCapability(webrtc::VideoCaptureCapability capability)
{
    LOCK_READ_SAFE_OBJ(_capturer);
    const auto& capturer = _capturer.constRef();
    capability = bestMatched(std::move(capability), capturer);
    LOCK_READ_SAFE_OBJ(_capability);
    if (capability != _capability.constRef()) {
        if (capturer && capturer->CaptureStarted()) {
            stop(capturer);
            start(capturer, capability);
        }
        _capability = std::move(capability);
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
    if (stats && _hasLastResolution) {
        const auto lastResolution = _lastResolution.load();
        stats->input_width = extractHiWord(lastResolution);
        stats->input_height = extractLoWord(lastResolution);
        return true;
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
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        const auto it = _broadcasters->find(sink);
        if (it != _broadcasters->end()) {
            it->second->updateSinkWants(wants);
        }
        else {
            auto adapter = std::make_unique<VideoSinkBroadcast>(sink, wants);
            _broadcasters->insert(std::make_pair(sink, std::move(adapter)));
            if (1U == _broadcasters->size()) {
                start(_capturer(), _capability());
            }
        }
    }
}

void CameraVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        if (_broadcasters->erase(sink) > 0U && _broadcasters->empty()) {
            stop(_capturer());
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
        _hasLastResolution = true;
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

bool CameraVideoSource::start(const rtc::scoped_refptr<CameraCapturer>& capturer,
                              const webrtc::VideoCaptureCapability& capability) const
{
    if (capturer) {
        const auto code = capturer->StartCapture(capability);
        if (0 != code) {
            logError(makeCapturerError(code, "failed to start capturer"));
            return false;
        }
    }
    return true;
}

bool CameraVideoSource::stop(const rtc::scoped_refptr<CameraCapturer>& capturer) const
{
    if (capturer) {
        const auto code = capturer->StopCapture();
        if (0 != code) {
            logError(makeCapturerError(code, "failed to stop capturer"));
            return false;
        }
    }
    return true;
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
                if (_hasLastResolution.exchange(false)) {
                    _lastResolution = 0ULL;
                }
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
