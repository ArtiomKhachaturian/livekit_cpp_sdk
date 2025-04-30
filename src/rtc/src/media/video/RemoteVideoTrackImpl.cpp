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
#include "RemoteVideoTrackImpl.h"

namespace LiveKitCpp
{

RemoteVideoTrackImpl::RemoteVideoTrackImpl(const TrackInfo& info,
                                           const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                           std::shared_ptr<VideoDeviceImpl> videoDevice,
                                           const std::weak_ptr<TrackManager>& trackManager)
    : Base(info, receiver, std::move(videoDevice), trackManager)
{
    switch (info._source) {
        case TrackSource::Camera:
            setContentHint(cameraContentHint());
            break;
        case TrackSource::ScreenShare:
            setContentHint(sharingContentHint());
            break;
        default:
            break;
    }
    auto parameters = rtpParameters();
    if (setDegradationPreference(degradationPreference(), parameters)) {
        setRtpParameters(parameters);
    }
}

void RemoteVideoTrackImpl::onDegradationPreferenceChanged(DegradationPreference preference)
{
    Base::onDegradationPreferenceChanged(preference);
    auto parameters = rtpParameters();
    if (setDegradationPreference(preference, parameters)) {
        setRtpParameters(parameters);
    }
}

} // namespace LiveKitCpp
