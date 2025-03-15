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
#include "CameraVideoTrack.h"
#include "CameraManager.h"
#include "CameraCapturer.h"

namespace LiveKitCpp
{

CameraVideoTrack::CameraVideoTrack(const std::string& id,
                                   webrtc::scoped_refptr<CameraVideoSource> source,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<webrtc::ObserverInterface>(logger)
    , _id(id)
    , _source(std::move(source))
    , _observers(_source ? _source->signalingThread() : std::weak_ptr<rtc::Thread>())
    , _state(webrtc::MediaStreamTrackInterface::TrackState::kEnded)
{
    if (_source) {
        if (webrtc::MediaSourceInterface::kEnded != _source->state()) {
            changeState(webrtc::MediaStreamTrackInterface::TrackState::kLive);
        }
        _source->RegisterObserver(this);
    }
}

CameraVideoTrack::~CameraVideoTrack()
{
    if (_source) {
        resetSourceCapturer();
        _source->UnregisterObserver(this);
        changeState(webrtc::MediaStreamTrackInterface::TrackState::kEnded);
    }
}

void CameraVideoTrack::setDevice(MediaDevice device)
{
    LOCK_WRITE_SAFE_OBJ(_device);
    if (_device->_guid != device._guid) {
        _device = std::move(device);
        if (_sinksCount > 0U) {
            setSourceCapturer(_device.constRef());
        }
    }
}

void CameraVideoTrack::setCapability(webrtc::VideoCaptureCapability capability)
{
    if (_source) {
        _source->setCapability(capability);
    }
}

void CameraVideoTrack::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                       const rtc::VideoSinkWants& wants)
{
    if (sink && _source) {
        if (0U == _sinksCount.fetch_add(1U)) {
            // create capturer
            setSourceCapturer(_device());
        }
        _source->AddOrUpdateSink(sink, wants);
    }
}

void CameraVideoTrack::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink && _source) {
        _source->RemoveSink(sink);
        if (1U == _sinksCount.fetch_sub(1U)) {
            // destroy capturer
            resetSourceCapturer();
        }
    }
}

webrtc::VideoTrackSourceInterface* CameraVideoTrack::GetSource() const
{
    return _source.get();
}

bool CameraVideoTrack::enabled() const
{
    return _source && _source->enabled();
}

bool CameraVideoTrack::set_enabled(bool enable)
{
    if (_source && _source->setEnabled(enable)) {
        _observers.invoke(&webrtc::ObserverInterface::OnChanged);
        return true;
    }
    return false;
}

void CameraVideoTrack::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.add(observer);
}

void CameraVideoTrack::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _observers.remove(observer);
}

void CameraVideoTrack::setSourceCapturer(MediaDevice deviceInfo)
{
    if (_source && CameraManager::available()) {
        bool ok = true;
        if (deviceInfo._guid.empty()) {
            ok = CameraManager::defaultDevice(deviceInfo);
        }
        if (ok) {
            _source->setCapturer(CameraManager::createCapturer(deviceInfo));
        }
    }
}

void CameraVideoTrack::resetSourceCapturer()
{
    if (_source) {
        _source->setCapturer(nullptr);
    }
}

void CameraVideoTrack::changeState(webrtc::MediaStreamTrackInterface::TrackState state)
{
    if (state != _state.exchange(state)) {
        _observers.invoke(&webrtc::ObserverInterface::OnChanged);
    }
}

std::string_view CameraVideoTrack::logCategory() const
{
    return CameraManager::logCategory();
}

void CameraVideoTrack::OnChanged()
{
    if (_source) {
        switch (_source->state()) {
            case webrtc::MediaSourceInterface::kEnded:
                changeState(webrtc::MediaStreamTrackInterface::TrackState::kEnded);
                break;
            default:
                changeState(webrtc::MediaStreamTrackInterface::TrackState::kLive);
                break;
        }
    }
}

} // namespace LiveKitCpp
