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
#pragma once // SetSdpObserver.h
#include "Listener.h"
#include <api/set_local_description_observer_interface.h>
#include <api/set_remote_description_observer_interface.h>
#include <type_traits>

namespace LiveKitCpp
{

class SetSdpListener;

template <class TObserveInterface>
class SetSdpObserver : public TObserveInterface
{
public:
    void setListener(SetSdpListener* listener = nullptr) { _listener = listener; }
protected:
    SetSdpObserver() = default;
    void process(webrtc::RTCError error) const;
private:
    static constexpr bool _local = std::is_same_v<TObserveInterface, webrtc::SetLocalDescriptionObserverInterface>;
    Bricks::Listener<SetSdpListener*> _listener;
};

class SetLocalSdpObserver : public SetSdpObserver<webrtc::SetLocalDescriptionObserverInterface>
{
public:
    SetLocalSdpObserver() = default;
    // webrtc::SetLocalDescriptionObserverInterface implementation
    void OnSetLocalDescriptionComplete(webrtc::RTCError error) final;
};

class SetRemoteSdpObserver : public SetSdpObserver<webrtc::SetRemoteDescriptionObserverInterface>
{
public:
    SetRemoteSdpObserver() = default;
    // webrtc::SetLocalDescriptionObserverInterface implementation
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) final;
};

} // namespace LiveKitCpp
