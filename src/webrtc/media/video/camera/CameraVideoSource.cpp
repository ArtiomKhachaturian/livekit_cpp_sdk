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
#include "CameraCapturerProxySink.h"
#include "Loggable.h"
#include "SafeScopedRefPtr.h"
#include "Utils.h"
#include <memory>
#include <unordered_map>

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

class CameraVideoSource::Impl : public Bricks::LoggableS<CameraCapturerProxySink>
{
    using Base = Bricks::LoggableS<CameraCapturerProxySink>;
    using Broadcasters = std::unordered_map<rtc::VideoSinkInterface<webrtc::VideoFrame>*,
                                            std::unique_ptr<VideoSinkBroadcast>>;
public:
    Impl(const std::shared_ptr<Bricks::Logger>& logger,
         const std::weak_ptr<rtc::Thread>& signalingThread);
    ~Impl() final { reset(); }
    void setDevice(MediaDevice device);
    void setCapability(webrtc::VideoCaptureCapability capability);
    void setEnabled(bool enabled);
    bool stats(webrtc::VideoTrackSourceInterface::Stats& s) const;
    webrtc::MediaSourceInterface::SourceState state() const noexcept { return _state; }
    void processConstraints(const webrtc::VideoTrackSourceConstraints& c);
    void requestCapturer();
    void resetCapturer();
    void notifyAboutChanges() { _observers.invoke(&webrtc::ObserverInterface::OnChanged); }
    // return true if need to request capturer
    bool addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants);
    // return true if need to reset capturer
    bool removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink);
    void registerObserver(webrtc::ObserverInterface* observer);
    void unregisterObserver(webrtc::ObserverInterface* observer);
    void reset();
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final { return CameraManager::logCategory(); }
private:
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      std::string_view guid = {});
    static webrtc::VideoCaptureCapability bestMatched(webrtc::VideoCaptureCapability capability,
                                                      const rtc::scoped_refptr<CameraCapturer>& capturer);
    void changeState(webrtc::MediaSourceInterface::SourceState state);
    bool startCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer,
                       const webrtc::VideoCaptureCapability& capability) const;
    bool stopCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer) const;
    void logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
                  const std::string& message, int code = 0) const;
    void logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer, const std::string& message) const;
    void destroyCapturer(); // non-threadsafe
    bool frameWanted() const;
    // impl. of CameraObserver
    void onStateChanged(CameraState state) final;
    // impl. of rtc::VideoSinkInterface<webrtc::VideoFrame>
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final;
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c) final;
private:
    AsyncListeners<webrtc::ObserverInterface*> _observers;
    Bricks::SafeObj<Broadcasters> _broadcasters;
    Bricks::SafeObj<MediaDevice> _device;
    SafeScopedRefPtr<CameraCapturer> _capturer;
    Bricks::SafeObj<webrtc::VideoCaptureCapability> _capability = CameraManager::defaultCapability();
    std::atomic<uint64_t> _lastResolution = 0ULL;
    std::atomic<webrtc::MediaSourceInterface::SourceState> _state = webrtc::MediaSourceInterface::kEnded;
};

CameraVideoSource::CameraVideoSource(std::weak_ptr<rtc::Thread> signalingThread,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : _thread(std::move(signalingThread))
    , _impl(std::make_shared<Impl>(logger, _thread))
{
}

CameraVideoSource::~CameraVideoSource()
{
    ImplCall::postOrInvoke(_thread, _impl, &Impl::reset);
}

void CameraVideoSource::setDevice(MediaDevice device)
{
    ImplCall::postOrInvoke(_thread, _impl, &Impl::setDevice, std::move(device));
}

void CameraVideoSource::setCapability(webrtc::VideoCaptureCapability capability)
{
    ImplCall::postOrInvoke(_thread, _impl, &Impl::setCapability, std::move(capability));
}

bool CameraVideoSource::setEnabled(bool enabled)
{
    if (enabled != _enabled.exchange(enabled)) {
        ImplCall::postOrInvoke(_thread, _impl, &Impl::setEnabled, enabled);
        _impl->notifyAboutChanges();
        return true;
    }
    return false;
}

bool CameraVideoSource::GetStats(Stats* stats)
{
    if (stats) {
        return _impl->stats(*stats);
    }
    return false;
}

void CameraVideoSource::ProcessConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    ImplCall::postOrInvoke(_thread, _impl, &Impl::processConstraints, c);
}

webrtc::MediaSourceInterface::SourceState CameraVideoSource::state() const
{
    return _impl->state();
}

void CameraVideoSource::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                        const rtc::VideoSinkWants& wants)
{
    if (_impl->addOrUpdateSink(sink, wants)) {
        ImplCall::postOrInvoke(_thread, _impl, &Impl::requestCapturer);
    }
}

void CameraVideoSource::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (_impl->removeSink(sink)) {
        ImplCall::postOrInvoke(_thread, _impl, &Impl::resetCapturer);
    }
}

void CameraVideoSource::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _impl->registerObserver(observer);
}

void CameraVideoSource::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _impl->unregisterObserver(observer);
}

CameraVideoSource::Impl::Impl(const std::shared_ptr<Bricks::Logger>& logger,
                              const std::weak_ptr<rtc::Thread>& signalingThread)
    : Base(logger)
    , _observers(signalingThread)
{
    CameraManager::defaultDevice(_device.ref());
}

void CameraVideoSource::Impl::setDevice(MediaDevice device)
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

void CameraVideoSource::Impl::setCapability(webrtc::VideoCaptureCapability capability)
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

void CameraVideoSource::Impl::setEnabled(bool enabled)
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->applyBlackFrames(!enabled);
    }
    _observers.invoke(&webrtc::ObserverInterface::OnChanged);
}

bool CameraVideoSource::Impl::stats(webrtc::VideoTrackSourceInterface::Stats& s) const
{
    const auto lastResolution = _lastResolution.load();
    if (lastResolution) {
        s.input_width = extractHiWord(lastResolution);
        s.input_height = extractLoWord(lastResolution);
        return true;
    }
    return false;
}

void CameraVideoSource::Impl::processConstraints(const webrtc::VideoTrackSourceConstraints& c)
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnConstraintsChanged(c);
    }
}

void CameraVideoSource::Impl::requestCapturer()
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

void CameraVideoSource::Impl::resetCapturer()
{
    LOCK_WRITE_SAFE_OBJ(_capturer);
    destroyCapturer();
}

bool CameraVideoSource::Impl::addOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
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
            return 1U == _broadcasters->size();
        }
    }
    return false;
}

bool CameraVideoSource::Impl::removeSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_broadcasters);
        if (_broadcasters->erase(sink) > 0U) {
            return _broadcasters->empty();
        }
    }
    return false;
}

void CameraVideoSource::Impl::registerObserver(webrtc::ObserverInterface* observer)
{
    _observers.add(observer);
}

void CameraVideoSource::Impl::unregisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.remove(observer);
}

void CameraVideoSource::Impl::reset()
{
    resetCapturer();
    _broadcasters({});
}

webrtc::VideoCaptureCapability CameraVideoSource::Impl::
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

webrtc::VideoCaptureCapability CameraVideoSource::Impl::
    bestMatched(webrtc::VideoCaptureCapability capability,
                const rtc::scoped_refptr<CameraCapturer>& capturer)
{
    if (capturer) {
        return bestMatched(std::move(capability), capturer->guid());
    }
    return bestMatched(std::move(capability));
}

void CameraVideoSource::Impl::changeState(webrtc::MediaSourceInterface::SourceState state)
{
    if (state != _state.exchange(state)) {
        switch (state) {
            case webrtc::MediaSourceInterface::SourceState::kEnded:
                _lastResolution = 0ULL;
                break;
            default:
                break;
        }
        notifyAboutChanges();
    }
}

bool CameraVideoSource::Impl::startCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer,
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

bool CameraVideoSource::Impl::stopCapturer(const rtc::scoped_refptr<CameraCapturer>& capturer) const
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

void CameraVideoSource::Impl::logError(const rtc::scoped_refptr<CameraCapturer>& capturer,
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

void CameraVideoSource::Impl::logVerbose(const rtc::scoped_refptr<CameraCapturer>& capturer,
                                         const std::string& message) const
{
    if (capturer && canLogVerbose()) {
        Base::logVerbose(CameraManager::formatLogMessage(capturer->CurrentDeviceName(), message));
    }
}

void CameraVideoSource::Impl::destroyCapturer()
{
    if (auto capturer = _capturer.take()) {
        stopCapturer(capturer);
        capturer->DeRegisterCaptureDataCallback();
        capturer->setObserver(nullptr);
    }
}

bool CameraVideoSource::Impl::frameWanted() const
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    return !_broadcasters->empty();
}

void CameraVideoSource::Impl::onStateChanged(CameraState state)
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

void CameraVideoSource::Impl::OnFrame(const webrtc::VideoFrame& frame)
{
    if (frame.video_frame_buffer()) {
        _lastResolution = clueToUint64(frame.width(), frame.height());
        LOCK_READ_SAFE_OBJ(_broadcasters);
        for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
            it->second->OnFrame(frame);
        }
    }
}

void CameraVideoSource::Impl::OnDiscardedFrame()
{
    LOCK_READ_SAFE_OBJ(_broadcasters);
    for (auto it = _broadcasters->begin(); it != _broadcasters->end(); ++it) {
        it->second->OnDiscardedFrame();
    }
}

void CameraVideoSource::Impl::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    processConstraints(c);
}

} // namespace LiveKitCpp
