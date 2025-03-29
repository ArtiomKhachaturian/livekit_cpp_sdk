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
#pragma once // AudioDeviceModuleListener.h

namespace LiveKitCpp
{

struct MediaDeviceInfo;

class AudioDeviceModuleListener
{
public:
    virtual void onRecordingChanged(const MediaDeviceInfo& /*device*/) {}
    virtual void onPlayoutChanged(const MediaDeviceInfo& /*device*/) {}
protected:
    virtual ~AudioDeviceModuleListener() = default;
};

} // namespace LiveKitCpp
