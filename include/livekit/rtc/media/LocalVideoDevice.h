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
#pragma once // CameraDevice.h
#include "livekit/rtc/media/VideoDevice.h"
#include "livekit/rtc/media/VideoOptions.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <memory>

namespace LiveKitCpp
{

class LocalVideoFilterPin;

class LocalVideoDevice : public VideoDevice
{
public:
    virtual void setDeviceInfo(MediaDeviceInfo info = {}) = 0;
    virtual MediaDeviceInfo deviceInfo() const = 0;
    virtual void setOptions(VideoOptions options = {}) = 0;
    virtual VideoOptions options() const = 0;
    virtual bool screencast() const = 0;
    // return output pin
    virtual void setFilter(LocalVideoFilterPin* inputPin) = 0;
    void resetFilter() { setFilter(nullptr); }
};

} // namespace LiveKitCpp
