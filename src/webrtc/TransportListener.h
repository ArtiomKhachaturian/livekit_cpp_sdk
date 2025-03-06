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
#pragma once // TransportListener.h
#include <api/jsep.h>
#include <memory>

namespace LiveKitCpp
{

enum class SignalTarget;

class TransportListener
{
public:
    virtual void onSdpCreated(SignalTarget target, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) = 0;
    virtual void onSdpCreationFailure(SignalTarget target, webrtc::SdpType type, webrtc::RTCError error) = 0;
    virtual void onSdpSet(SignalTarget target, bool local) = 0;
    virtual void onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error) = 0;
    virtual void onSetConfigurationError(SignalTarget target, webrtc::RTCError error) = 0;
protected:
    virtual ~TransportListener() = default;
};

} // namespace LiveKitCpp
