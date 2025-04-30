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
#include "LocalVideoTrackImpl.h"

namespace {

// see enum class TrackSource
inline std::string videoLabel(bool sharing) {
    return sharing ? "screen_share" : "camera";
}

}

namespace LiveKitCpp
{

LocalVideoTrackImpl::LocalVideoTrackImpl(EncryptionType encryption,
                                         std::shared_ptr<LocalVideoDeviceImpl> device,
                                         const std::weak_ptr<TrackManager>& trackManager)
    : Base(videoLabel(device && device->screencast()), encryption, device, trackManager)
{
}

TrackSource LocalVideoTrackImpl::source() const
{
    if (mediaDevice() && mediaDevice()->screencast()) {
        return TrackSource::ScreenShare;
    }
    return TrackSource::Camera;
}

bool LocalVideoTrackImpl::fillRequest(AddTrackRequest* request) const
{
    if (Base::fillRequest(request)) {
        request->_type = type();
        request->_source = source();
        return true;
    }
    return false;
}

void LocalVideoTrackImpl::setDeviceInfo(MediaDeviceInfo info)
{
    if (const auto& dev = mediaDevice()) {
        dev->setDeviceInfo(std::move(info));
    }
}

MediaDeviceInfo LocalVideoTrackImpl::deviceInfo() const
{
    if (const auto& dev = mediaDevice()) {
        return dev->deviceInfo();
    }
    return {};
}

void LocalVideoTrackImpl::setOptions(VideoOptions options)
{
    if (const auto& dev = mediaDevice()) {
        dev->setOptions(std::move(options));
    }
}

VideoOptions LocalVideoTrackImpl::options() const
{
    if (const auto& dev = mediaDevice()) {
        return dev->options();
    }
    return {};
}

void LocalVideoTrackImpl::setFilter(LocalVideoFilterPin* inputPin)
{
    if (const auto& dev = mediaDevice()) {
        dev->setFilter(inputPin);
    }
}

bool LocalVideoTrackImpl::updateSenderInitialParameters(webrtc::RtpParameters& parameters) const
{
    const auto b1 = Base::updateSenderInitialParameters(parameters);
    const auto b2 = setDegradationPreference(degradationPreference(), parameters);
    return b1 || b2;
}

void LocalVideoTrackImpl::onDegradationPreferenceChanged(DegradationPreference preference)
{
    Base::onDegradationPreferenceChanged(preference);
    auto parameters = rtpParameters();
    if (setDegradationPreference(preference, parameters)) {
        setRtpParameters(parameters);
    }
}

} // namespace LiveKitCpp
