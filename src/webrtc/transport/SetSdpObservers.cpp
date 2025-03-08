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
#include "SetSdpObservers.h"
#include "SetSdpListener.h"

namespace LiveKitCpp
{

template<class TObserveInterface>
void SetSdpObserver<TObserveInterface>::process(webrtc::RTCError error) const
{
    if (error.ok()) {
        _listener.invoke(&SetSdpListener::onCompleted, _local);
    }
    else {
        _listener.invoke(&SetSdpListener::onFailure, _local, std::move(error));
    }
}

void SetLocalSdpObserver::OnSetLocalDescriptionComplete(webrtc::RTCError error)
{
    process(std::move(error));
}

void SetRemoteSdpObserver::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    process(std::move(error));
}

} // namespace LiveKitCpp
