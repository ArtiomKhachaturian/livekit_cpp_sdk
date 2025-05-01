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

void LocalVideoTrackImpl::setDegradationPreference(DegradationPreference preference)
{
    if (exchangeVal(preference, _degradationPreference)) {
        auto parameters = rtpParameters();
        if (setDegradationPreference(preference, parameters)) {
            setRtpParameters(parameters);
        }
    }
}

std::optional<int> LocalVideoTrackImpl::maxBitrateBps() const
{
    return optionalValue(_maxBitrateBps);
}

void LocalVideoTrackImpl::setMaxBitrateBps(const std::optional<int>& bps)
{
    const auto v = value(bps);
    if (exchangeVal(v, _maxBitrateBps)) {
        auto parameters = rtpParameters();
        if (setMaxBitrateBps(optionalValue(v), parameters)) {
            setRtpParameters(parameters);
        }
    }
}

std::optional<int> LocalVideoTrackImpl::minBitrateBps() const
{
    return optionalValue(_minBitrateBps);
}

void LocalVideoTrackImpl::setMinBitrateBps(const std::optional<int>& bps)
{
    const auto v = value(bps);
    if (exchangeVal(v, _minBitrateBps)) {
        auto parameters = rtpParameters();
        if (setMinBitrateBps(optionalValue(v), parameters)) {
            setRtpParameters(parameters);
        }
    }
}

std::optional<int> LocalVideoTrackImpl::maxFramerate() const
{
    return optionalValue(_maxFramerate);
}

void LocalVideoTrackImpl::setMaxFramerate(const std::optional<int>& fps)
{
    const auto v = value(fps);
    if (exchangeVal(v, _maxFramerate)) {
        auto parameters = rtpParameters();
        if (setMaxFramerate(optionalValue(v), parameters)) {
            setRtpParameters(parameters);
        }
    }
}

bool LocalVideoTrackImpl::updateSenderInitialParameters(webrtc::RtpParameters& parameters) const
{
    const auto b1 = Base::updateSenderInitialParameters(parameters);
    const auto b2 = setDegradationPreference(degradationPreference(), parameters);
    const auto b3 = setMaxBitrateBps(maxBitrateBps(), parameters);
    const auto b4 = setMinBitrateBps(minBitrateBps(), parameters);
    const auto b5 = setMaxFramerate(maxFramerate(), parameters);
    return b1 || b2 || b3 || b4 || b5;
}

std::optional<int> LocalVideoTrackImpl::optionalValue(int v)
{
    if (v == _noValue) {
        return std::nullopt;
    }
    return v;
}

int LocalVideoTrackImpl::value(const std::optional<int>& v)
{
    if (v.has_value() && v.value() >= 0) {
        return v.value();
    }
    return _noValue;
}

bool LocalVideoTrackImpl::setMaxBitrateBps(const std::optional<int>& bps,
                                           webrtc::RtpParameters& parameters)
{
    bool changed = false;
    if (!parameters.encodings.empty()) {
        for (auto& encoding : parameters.encodings) {
            if (setMaxBitrateBps(bps, encoding)) {
                changed = true;
            }
        }
    }
    return changed;
}

bool LocalVideoTrackImpl::setMaxBitrateBps(const std::optional<int>& bps,
                                           webrtc::RtpEncodingParameters& parameters)
{
    if (bps != parameters.max_bitrate_bps) {
        parameters.max_bitrate_bps = bps;
        return true;
    }
    return false;
}

bool LocalVideoTrackImpl::setMinBitrateBps(const std::optional<int>& bps,
                                           webrtc::RtpParameters& parameters)
{
    bool changed = false;
    if (!parameters.encodings.empty()) {
        for (auto& encoding : parameters.encodings) {
            if (setMinBitrateBps(bps, encoding)) {
                changed = true;
            }
        }
    }
    return changed;
}

bool LocalVideoTrackImpl::setMinBitrateBps(const std::optional<int>& bps,
                                           webrtc::RtpEncodingParameters& parameters)
{
    if (bps != parameters.min_bitrate_bps) {
        parameters.min_bitrate_bps = bps;
        return true;
    }
    return false;
}

bool LocalVideoTrackImpl::setMaxFramerate(const std::optional<int>& fps,
                                           webrtc::RtpParameters& parameters)
{
    bool changed = false;
    if (!parameters.encodings.empty()) {
        for (auto& encoding : parameters.encodings) {
            if (setMaxFramerate(fps, encoding)) {
                changed = true;
            }
        }
    }
    return changed;
}

bool LocalVideoTrackImpl::setMaxFramerate(const std::optional<int>& fps,
                                           webrtc::RtpEncodingParameters& parameters)
{
    if (fps != parameters.max_framerate) {
        parameters.max_framerate = fps;
        return true;
    }
    return false;
}

bool LocalVideoTrackImpl::setDegradationPreference(DegradationPreference preference,
                                                   webrtc::RtpParameters& parameters)
{
    std::optional<webrtc::DegradationPreference> rtcPreference;
    switch (preference) {
        case DegradationPreference::Default:
            break;
        case DegradationPreference::Disabled:
            rtcPreference = webrtc::DegradationPreference::DISABLED;
            break;
        case DegradationPreference::MaintainFramerate:
            rtcPreference = webrtc::DegradationPreference::MAINTAIN_FRAMERATE;
            break;
        case DegradationPreference::MaintainResolution:
            rtcPreference = webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
            break;
        case DegradationPreference::Balanced:
            rtcPreference = webrtc::DegradationPreference::BALANCED;
            break;
    }
    if (parameters.degradation_preference != rtcPreference) {
        parameters.degradation_preference = std::move(rtcPreference);
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
