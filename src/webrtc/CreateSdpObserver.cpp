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
#include "CreateSdpObserver.h"
#include "CreateSdpListener.h"

namespace LiveKitCpp
{

CreateSdpObserver::CreateSdpObserver(webrtc::SdpType type)
    : _type(type)
{
}

void CreateSdpObserver::setListener(CreateSdpListener* listener)
{
    _listener = listener;
}

void CreateSdpObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
    if (desc) {
        _listener.invoke(&CreateSdpListener::onSuccess,
                         std::unique_ptr<webrtc::SessionDescriptionInterface>(desc));
    }
}

void CreateSdpObserver::OnFailure(webrtc::RTCError error)
{
    if (!error.ok()) {
        _listener.invoke(&CreateSdpListener::onFailure, _type, std::move(error));
    }
}

} // namespace LiveKitCpp
