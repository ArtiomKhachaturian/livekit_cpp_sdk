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
#pragma once // RemoteVideoTrackImpl.h
#include "RemoteTrackImpl.h"
#include "VideoTrackImpl.h"
#include "RemoteVideoTrack.h"


namespace LiveKitCpp
{

class RemoteVideoTrackImpl : public RemoteTrackImpl<VideoTrackImpl<RemoteVideoTrack>>
{
    using Base = RemoteTrackImpl<VideoTrackImpl<RemoteVideoTrack>>;
public:
    RemoteVideoTrackImpl(const TrackInfo& info,
                         const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                         webrtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack,
                         TrackManager* manager);
    // impl. of RemoteVideoTrack
    std::string sid() const final { return info()._sid; }
    uint32_t originalWidth() const final { return info()._width; }
    uint32_t originalHeight() const final { return info()._height; }
    std::vector<VideoLayer> layers() const final { return info()._layers; }
    std::vector<SimulcastCodecInfo> codecs() const final { return info()._codecs; }
    std::string mime() const final { return info()._mimeType; }
    std::string stream() const final { return info()._stream; }
};

} // namespace LiveKitCpp
