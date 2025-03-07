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
#pragma once // Transport.h
#include "CreateSdpListener.h"
#include "SetSdpListener.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/SignalTarget.h"
#include "rtc/ICEServer.h"
#include <api/peer_connection_interface.h>
#include <memory>
#include <optional>
#include <vector>

namespace webrtc {
class PeerConnectionObserver;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;
class CreateSdpObserver;
class SetLocalSdpObserver;
class SetRemoteSdpObserver;
class TransportListener;

class Transport : private CreateSdpListener,
                  private SetSdpListener
{
public:
    ~Transport() override;
    static std::unique_ptr<Transport> create(bool primary, SignalTarget target,
                                             TransportListener* listener,
                                             const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
    bool primary() const noexcept { return _primary; }
    SignalTarget target() const noexcept { return _target; }
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    void createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
private:
    Transport(bool primary, SignalTarget target, TransportListener* listener,
              webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc);
    // impl. of CreateSdpObserver
    void onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onFailure(webrtc::SdpType type, webrtc::RTCError error) final;
    // impl. of SetSdpListener
    void onCompleted(bool local) final;
    void onFailure(bool local, webrtc::RTCError error) final;
private:
    const bool _primary;
    const SignalTarget _target;
    TransportListener* const _listener;
    const webrtc::scoped_refptr<CreateSdpObserver> _offerCreationObserver;
    const webrtc::scoped_refptr<CreateSdpObserver> _answerCreationObserver;
    const webrtc::scoped_refptr<SetLocalSdpObserver> _setLocalSdpObserver;
    const webrtc::scoped_refptr<SetRemoteSdpObserver> _setRemoteSdpObserver;
    const webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
};

} // namespace LiveKitCpp
