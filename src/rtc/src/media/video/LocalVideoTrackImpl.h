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
#pragma once // LocalVideoTrackImpl.h
#include "LocalTrackImpl.h"
#include "LocalVideoDeviceImpl.h"
#include "VideoTrackImpl.h"
#include "livekit/rtc/media/LocalVideoTrack.h"
#include <limits>

namespace LiveKitCpp
{

class LocalVideoTrackImpl : public LocalTrackImpl<VideoTrackImpl<LocalVideoDeviceImpl, LocalVideoTrack>>
{
    using Base = LocalTrackImpl<VideoTrackImpl<LocalVideoDeviceImpl, LocalVideoTrack>>;
public:
    LocalVideoTrackImpl(EncryptionType encryption,
                        std::shared_ptr<LocalVideoDeviceImpl> device,
                        const std::weak_ptr<TrackManager>& trackManager);
    // impl. of LocalTrack
    TrackSource source() const final;
    bool fillRequest(AddTrackRequest* request) const final;
    // impl. of LocalVideoTrack
    void setDeviceInfo(MediaDeviceInfo info) final;
    MediaDeviceInfo deviceInfo() const final;
    void setOptions(VideoOptions options) final;
    VideoOptions options() const final;
    void setFilter(LocalVideoFilterPin* inputPin) final;
    DegradationPreference degradationPreference() const final { return _degradationPreference; }
    void setDegradationPreference(DegradationPreference preference) final;
    std::optional<int> maxBitrateBps() const final;
    void setMaxBitrateBps(const std::optional<int>& bps) final;
    std::optional<int> minBitrateBps() const final;
    void setMinBitrateBps(const std::optional<int>& bps) final;
    std::optional<int> maxFramerate() const final;
    void setMaxFramerate(const std::optional<int>& fps) final;
    VideoScalabilityMode scalabilityMode() const final;
    void setScalabilityMode(VideoScalabilityMode mode) final;
protected:
    // overrides of VideoTrackImpl<>
    bool updateSenderInitialParameters(webrtc::RtpParameters& parameters) const final;
private:
    static std::optional<int> optionalValue(int v);
    static int value(const std::optional<int>& v);
    static bool setMaxBitrateBps(const std::optional<int>& bps, webrtc::RtpParameters& parameters);
    static bool setMaxBitrateBps(const std::optional<int>& bps, webrtc::RtpEncodingParameters& parameters);
    static bool setMinBitrateBps(const std::optional<int>& bps, webrtc::RtpParameters& parameters);
    static bool setMinBitrateBps(const std::optional<int>& bps, webrtc::RtpEncodingParameters& parameters);
    static bool setMaxFramerate(const std::optional<int>& fps, webrtc::RtpParameters& parameters);
    static bool setMaxFramerate(const std::optional<int>& fps, webrtc::RtpEncodingParameters& parameters);
    static bool setScalabilityMode(VideoScalabilityMode mode, webrtc::RtpParameters& parameters);
    static bool setScalabilityMode(VideoScalabilityMode mode, webrtc::RtpEncodingParameters& parameters);
    static bool setDegradationPreference(DegradationPreference preference, webrtc::RtpParameters& parameters);
private:
    static constexpr int _noValue = std::numeric_limits<int>::min();
    std::atomic<DegradationPreference> _degradationPreference = DegradationPreference::Default;
    std::atomic<int> _maxBitrateBps = _noValue;
    std::atomic<int> _minBitrateBps = _noValue;
    std::atomic<int> _maxFramerate = _noValue;
    std::atomic<VideoScalabilityMode> _scalabilityMode = VideoScalabilityMode::Auto;
};
	
} // namespace LiveKitCpp
