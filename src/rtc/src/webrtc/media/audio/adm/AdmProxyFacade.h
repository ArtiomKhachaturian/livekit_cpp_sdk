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
#pragma once // AdmProxyFacade.h
#include "AdmProxyState.h"

namespace webrtc {
class AudioTrackSinkInterface;
}

namespace LiveKitCpp
{

class AdmProxyListener;

class AdmProxyFacade
{
public:
    virtual void registerRecordingSink(webrtc::AudioTrackSinkInterface* sink, bool reg) = 0;
    virtual void registerRecordingListener(AdmProxyListener* l, bool reg) = 0;
    virtual void registerPlayoutListener(AdmProxyListener* l, bool reg) = 0;
    virtual const AdmProxyState& recordingState() const = 0;
    virtual const AdmProxyState& playoutState() const = 0;
protected:
    virtual ~AdmProxyFacade() = default;
};

} // namespace LiveKitCpp
