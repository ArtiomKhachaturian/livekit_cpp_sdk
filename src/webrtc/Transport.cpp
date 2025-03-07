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
#include "Transport.h"
#include "Logger.h"
#include "TransportListener.h"
#include "CreateSdpObserver.h"
#include "SetSdpObservers.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

Transport::Transport(bool primary, SignalTarget target, TransportListener* listener,
                     webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc)
    : _primary(primary)
    , _target(target)
    , _listener(listener)
    , _offerCreationObserver(webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kOffer))
    , _answerCreationObserver(webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kAnswer))
    , _setLocalSdpObserver(webrtc::make_ref_counted<SetLocalSdpObserver>())
    , _setRemoteSdpObserver(webrtc::make_ref_counted<SetRemoteSdpObserver>())
    , _pc(std::move(pc))
{
    _offerCreationObserver->setListener(this);
    _answerCreationObserver->setListener(this);
    _setLocalSdpObserver->setListener(this);
    _setRemoteSdpObserver->setListener(this);
}

Transport::~Transport()
{
    _offerCreationObserver->setListener(nullptr);
    _answerCreationObserver->setListener(nullptr);
    _setLocalSdpObserver->setListener(nullptr);
    _setRemoteSdpObserver->setListener(nullptr);
}

std::unique_ptr<Transport> Transport::create(bool primary, SignalTarget target,
                                             TransportListener* listener,
                                             const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
{
    std::unique_ptr<Transport> transport;
    if (listener && pcf) {
        auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(listener));
        if (pc.ok()) {
            transport.reset(new Transport(primary, target, listener, std::move(pc.MoveValue())));
        }
        else if (const auto logger = pcf->logger()) {
            logger->logError(pc.error().message(), "CreatePeerConnection");
        }
    }
    return transport;
}

void Transport::createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    _pc->CreateOffer(_offerCreationObserver.get(), options);
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    _pc->CreateAnswer(_answerCreationObserver.get(), options);
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        _pc->SetLocalDescription(std::move(desc), _setLocalSdpObserver);
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        _pc->SetRemoteDescription(std::move(desc), _setRemoteSdpObserver);
    }
}

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    auto res = _pc->SetConfiguration(config);
    if (!res.ok()) {
        _listener->onSetConfigurationError(this, std::move(res));
    }
    return res.ok();
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    _listener->onSdpCreated(this, std::move(desc));
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    _listener->onSdpCreationFailure(this, type, std::move(error));
}

void Transport::onCompleted(bool local)
{
    const auto desc = local ? _pc->local_description() : _pc->remote_description();
    _listener->onSdpSet(this, local, desc);
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    _listener->onSdpSetFailure(this, local, std::move(error));
}

} // namespace LiveKitCpp
