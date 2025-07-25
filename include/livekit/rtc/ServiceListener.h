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
#pragma once // ServiceListener.h

namespace LiveKitCpp
{

struct MediaDeviceInfo;

class ServiceListener
{
public:
    virtual void onAudioRecordingStarted() {}
    virtual void onAudioRecordingStopped() {}
    virtual void onAudioPlayoutStarted() {}
    virtual void onAudioPlayoutStopped() {}
    virtual void onAudioRecordingEnabled(bool /*enabled*/) {}
    virtual void onAudioPlayoutEnabled(bool /*enabled*/) {}
    virtual void onAudioRecordingVolumeChanged(double /*volume*/) {}
    virtual void onAudioPlayoutVolumeChanged(double /*volume*/) {}
    virtual void onAudioRecordingDeviceChanged(const MediaDeviceInfo& /*info*/) {}
    virtual void onAudioPlayoutDeviceChanged(const MediaDeviceInfo& /*info*/) {}
protected:
    virtual ~ServiceListener() = default;
};

} // namespace LiveKitCpp
