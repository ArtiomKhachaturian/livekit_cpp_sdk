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
#include "CameraCapturerProxy.h"
#include "CameraCapturer.h"
#include "CameraCapturerProxySink.h"

namespace LiveKitCpp
{

CameraCapturerProxy::CameraCapturerProxy(rtc::scoped_refptr<CameraCapturer> impl)
    : _impl(std::move(impl))
{
    _impl->RegisterCaptureDataCallback(this);
    _impl->setObserver(this);
}

CameraCapturerProxy::~CameraCapturerProxy()
{
    _impl->DeRegisterCaptureDataCallback();
    _impl->setObserver(nullptr);
}

std::shared_ptr<CameraCapturerProxy> CameraCapturerProxy::create(rtc::scoped_refptr<CameraCapturer> impl)
{
    std::shared_ptr<CameraCapturerProxy> proxy;
    if (impl) {
        proxy.reset(new CameraCapturerProxy(std::move(impl)));
    }
    return proxy;
}

bool CameraCapturerProxy::started() const
{
    return _impl->CaptureStarted();
}

bool CameraCapturerProxy::captureCapability(webrtc::VideoCaptureCapability& capability) const
{
    LOCK_READ_SAFE_OBJ(_activeCapability);
    if (_activeCapability->has_value()) {
        capability = _activeCapability->value();
        return true;
    }
    return false;
}

int32_t CameraCapturerProxy::startCapture(const webrtc::VideoCaptureCapability& capability,
                                          CameraCapturerProxySink* sink)
{
    int32_t result = -1;
    if (sink) {
        LOCK_WRITE_SAFE_OBJ(_activeCapability);
        if (capability == _activeCapability->value_or(capability) &&
            Bricks::ok(_sinks.add(sink))) {
            if (!_activeCapability->has_value()) {
                result = _impl->StartCapture(capability);
                if (0 == result) {
                    _activeCapability = capability;
                }
            }
            else {
                sink->onStateChanged(CameraState::Starting);
                if (_impl->CaptureStarted()) {
                    sink->onStateChanged(CameraState::Started);
                    result = 0;
                }
                else {
                    sink->onStateChanged(CameraState::Stopped);
                }
            }
            if (0 != result) {
                _sinks.remove(sink);
            }
        }
    }
    return result;
}

int32_t CameraCapturerProxy::stopCapture(CameraCapturerProxySink* sink)
{
    int32_t result = -1;
    if (Bricks::ok(_sinks.remove(sink))) {
        result = 0;
        sink->onStateChanged(CameraState::Stopping);
        if (_sinks.empty()) {
            result = _impl->StopCapture();
            _activeCapability(std::nullopt);
        }
        sink->onStateChanged(CameraState::Stopped);
    }
    return result;
}

const char* CameraCapturerProxy::currentDeviceName() const
{
    return _impl->CurrentDeviceName();
}

void CameraCapturerProxy::onStateChanged(CameraState state)
{
    _sinks.invoke(&CameraCapturerProxySink::onStateChanged, state);
}

void CameraCapturerProxy::OnFrame(const webrtc::VideoFrame& frame)
{
    _sinks.invoke(&CameraCapturerProxySink::OnFrame, frame);
}

void CameraCapturerProxy::OnDiscardedFrame()
{
    _sinks.invoke(&CameraCapturerProxySink::OnDiscardedFrame);
}

void CameraCapturerProxy::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& constraints)
{
    _sinks.invoke(&CameraCapturerProxySink::OnConstraintsChanged, constraints);
}

} // LiveKitCpp
