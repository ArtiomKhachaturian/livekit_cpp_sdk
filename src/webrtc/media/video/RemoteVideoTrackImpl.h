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

class RemoteVideoTrackImpl : public RemoteTrackImpl<webrtc::VideoTrackInterface,
                                                    VideoTrackImpl<RemoteVideoTrack>>
{
    using Base = RemoteTrackImpl<webrtc::VideoTrackInterface, VideoTrackImpl<RemoteVideoTrack>>;
public:
    RemoteVideoTrackImpl(TrackManager* manager, const TrackInfo& info,
                         const webrtc::scoped_refptr<webrtc::VideoTrackInterface>& track);
    RemoteVideoTrackImpl(TrackManager* manager, const TrackInfo& info,
                         webrtc::VideoTrackInterface* track);
    ~RemoteVideoTrackImpl() final;
    // impl. of RemoteVideoTrack
    uint32_t originalWidth() const final { return info()._width; }
    uint32_t originalHeight() const final { return info()._height; }
    std::vector<VideoLayer> layers() const final { return info()._layers; }
    std::vector<SimulcastCodecInfo> codecs() const final { return info()._codecs; }
    std::string mime() const final { return info()._mimeType; }
    std::string stream() const final { return info()._stream; }
    EncryptionType encryption() const final { return info()._encryption; }
    BackupCodecPolicy backupCodecPolicy() const final { return info()._backupCodecPolicy; }
protected:
    rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack() const final;
};

} // namespace LiveKitCpp
