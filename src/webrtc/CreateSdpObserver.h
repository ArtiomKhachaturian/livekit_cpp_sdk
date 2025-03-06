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
#pragma once // CreateSdpObserver.h
#include "Listener.h"
#include <api/jsep.h>

namespace LiveKitCpp
{

class CreateSdpListener;

class CreateSdpObserver : public webrtc::CreateSessionDescriptionObserver
{
public:
    CreateSdpObserver(webrtc::SdpType type);
    void setListener(CreateSdpListener* listener = nullptr);
    // webrtc::CreateSessionDescriptionObserver implementation.
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) final;
    void OnFailure(webrtc::RTCError error) final;
private:
    const webrtc::SdpType _type;
    Listener<CreateSdpListener*> _listener;
};

} // namespace LiveKitCpp
