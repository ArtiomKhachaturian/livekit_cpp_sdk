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
#pragma once // MediaDevice.h
#include <string>

namespace LiveKitCpp
{

class MediaEventsListener;

class MediaDevice
{
public:
    virtual ~MediaDevice() = default;
    virtual bool audio() const = 0;
    virtual bool addListener(MediaEventsListener* listener) = 0;
    virtual bool removeListener(MediaEventsListener* listener) = 0;
    // live or ended, a device will never be live again after becoming ended
    virtual bool live() const = 0;
    // device unique ID
    virtual std::string id() const = 0;
    // mute/unmute state
    virtual void mute(bool mute = true) = 0;
    virtual bool muted() const = 0;
    void unmute() { mute(false); }
};

} // namespace LiveKitCpp
