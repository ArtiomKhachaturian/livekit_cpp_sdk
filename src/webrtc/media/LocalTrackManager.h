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
#pragma once // LocalTrackFactory.h
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <api/audio_options.h>
#include <string>

namespace LiveKitCpp
{

class LocalTrack;

class LocalTrackManager
{
public:
    virtual bool add(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) = 0;
    virtual bool remove(webrtc::scoped_refptr<webrtc::RtpSenderInterface> sender) = 0;
    virtual webrtc::scoped_refptr<webrtc::AudioTrackInterface> createAudio(const std::string& label,
                                                                           const cricket::AudioOptions& options = {}) = 0;
    virtual void notifyAboutEnabledChanges(const LocalTrack& track) = 0;
protected:
    virtual ~LocalTrackManager() = default;
};

} // namespace LiveKitCpp
