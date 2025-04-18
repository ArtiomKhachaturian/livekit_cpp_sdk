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
#include "LocalWebRtcTrack.h"
#include "VideoFrameBuffer.h"

namespace LiveKitCpp
{

LocalWebRtcTrack::LocalWebRtcTrack(const std::string& id,
                                   webrtc::scoped_refptr<AsyncVideoSource> source)
    : _id(id)
    , _source(std::move(source))
{
}

LocalWebRtcTrack::~LocalWebRtcTrack()
{
    close();
}

void LocalWebRtcTrack::close()
{
    if (_source) {
        _source->close();
    }
}

void LocalWebRtcTrack::setDeviceInfo(MediaDeviceInfo info)
{
    if (_source) {
        _source->setDeviceInfo(std::move(info));
    }
}

MediaDeviceInfo LocalWebRtcTrack::deviceInfo() const
{
    if (_source) {
        return _source->deviceInfo();
    }
    return {};
}

void LocalWebRtcTrack::setOptions(VideoOptions options)
{
    if (_source) {
        _source->setOptions(std::move(options));
    }
}

VideoOptions LocalWebRtcTrack::options() const
{
    if (_source) {
        return _source->options();
    }
    return {};
}

void LocalWebRtcTrack::addListener(MediaDeviceListener* listener)
{
    if (_source) {
        _source->addListener(listener);
    }
}

void LocalWebRtcTrack::removeListener(MediaDeviceListener* listener)
{
    if (_source) {
        _source->removeListener(listener);
    }
}

webrtc::VideoTrackInterface::ContentHint LocalWebRtcTrack::content_hint() const
{
    if (_source) {
        return map(_source->contentHint());
    }
    return webrtc::VideoTrackInterface::ContentHint::kNone;
}

void LocalWebRtcTrack::set_content_hint(ContentHint hint)
{
    if (_source) {
        _source->setContentHint(map(hint));
    }
}

void LocalWebRtcTrack::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                                       const rtc::VideoSinkWants& wants)
{
    if (_source) {
        _source->AddOrUpdateSink(sink, wants);
    }
}

void LocalWebRtcTrack::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink)
{
    if (_source) {
        _source->RemoveSink(sink);
    }
}

webrtc::VideoTrackSourceInterface* LocalWebRtcTrack::GetSource() const
{
    return _source.get();
}

bool LocalWebRtcTrack::enabled() const
{
    return _source->enabled();
}

bool LocalWebRtcTrack::set_enabled(bool enable)
{
    return _source && _source->setEnabled(enable);
}

webrtc::MediaStreamTrackInterface::TrackState LocalWebRtcTrack::state() const
{
    if (_source) {
        switch (_source->state()) {
            case webrtc::MediaSourceInterface::kEnded:
                return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
            default:
                break;
        }
        return webrtc::MediaStreamTrackInterface::TrackState::kLive;
    }
    return webrtc::MediaStreamTrackInterface::TrackState::kEnded;
}

void LocalWebRtcTrack::RegisterObserver(webrtc::ObserverInterface* observer)
{
    if (_source) {
        _source->RegisterObserver(observer);
    }
}

void LocalWebRtcTrack::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    if (_source) {
        _source->UnregisterObserver(observer);
    }
}

} // namespace LiveKitCpp
