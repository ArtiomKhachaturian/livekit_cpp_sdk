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
#include "CameraVideoSource.h"

namespace LiveKitCpp
{

CameraVideoTrack::CameraVideoTrack(const std::string& id,
                                   webrtc::scoped_refptr<CameraVideoSource> source,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<webrtc::ObserverInterface>(logger)
    , _id(id)
    , _source(std::move(source))
    , _observers(_source ? _source->signalingQueue() : std::weak_ptr<webrtc::TaskQueueBase>())
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
        _source->UnregisterObserver(this);
        changeState(webrtc::MediaStreamTrackInterface::TrackState::kEnded);
    }
    close();
}

void CameraVideoTrack::close()
{
    if (_source) {
        _source->close();
    }
}

void CameraVideoTrack::setDevice(MediaDevice device)
{
    if (_source) {
        _source->setDevice(std::move(device));
    }
}

MediaDevice CameraVideoTrack::device() const
{
    if (_source) {
        return _source->device();
    }
    return {};
}

void CameraVideoTrack::setCapability(const webrtc::VideoCaptureCapability& capability)
{
    if (_source) {
        _source->setCapability(capability);
    }
}

webrtc::VideoCaptureCapability CameraVideoTrack::capability() const
{
    if (_source) {
        return _source->capability();
    }
    return {};
}

void CameraVideoTrack::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                       const rtc::VideoSinkWants& wants)
{
    if (sink && _source) {
        _source->AddOrUpdateSink(sink, wants);
    }
}

void CameraVideoTrack::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (sink && _source) {
        _source->RemoveSink(sink);
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
