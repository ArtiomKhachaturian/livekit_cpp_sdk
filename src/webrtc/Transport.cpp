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

// https://github.com/livekit/client-sdk-js/blob/main/src/room/PCTransport.ts

namespace LiveKitCpp
{

Transport::Transport(bool primary, SignalTarget target, TransportListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
    : BaseTransport(pcf ? pcf->logger() : std::shared_ptr<Bricks::Logger>())
    , _primary(primary)
    , _target(target)
    , _listener(listener)
{
    if (pcf) {
        auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(this));
        if (pc.ok()) {
            _offerCreationObserver = webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kOffer);
            _answerCreationObserver = webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kAnswer);
            _setLocalSdpObserver = webrtc::make_ref_counted<SetLocalSdpObserver>();
            _setRemoteSdpObserver = webrtc::make_ref_counted<SetRemoteSdpObserver>();
            _offerCreationObserver->setListener(this);
            _answerCreationObserver->setListener(this);
            _setLocalSdpObserver->setListener(this);
            _setRemoteSdpObserver->setListener(this);
            _pc = std::move(pc.MoveValue());
        }
        else {
            logWebRTCError(pc.error());
        }
    }
}

Transport::~Transport()
{
    if (_offerCreationObserver) {
        _offerCreationObserver->setListener(nullptr);
    }
    if (_answerCreationObserver) {
        _answerCreationObserver->setListener(nullptr);
    }
    if (_setLocalSdpObserver) {
        _setLocalSdpObserver->setListener(nullptr);
    }
    if (_setRemoteSdpObserver) {
        _setRemoteSdpObserver->setListener(nullptr);
    }
    if (_pc) {
        _pc->Close();
    }
}

void Transport::createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (_pc) {
        const auto state = _pc->signaling_state();
        if (webrtc::PeerConnectionInterface::SignalingState::kClosed == state) {
            logWarning("could not createOffer with closed peer connection");
        }
        else {
            if (options.ice_restart) {
                setRestartingIce(true);
            }
            if (webrtc::PeerConnectionInterface::SignalingState::kHaveLocalOffer == state) {
                // we're waiting for the peer to accept our offer, so we'll just wait
                // the only exception to this is when ICE restart is needed
                const auto currentSD = _pc->remote_description();
                if (options.ice_restart && currentSD) {
                    // TODO: handle when ICE restart is needed but we don't have a remote description
                    // the best thing to do is to recreate the peerconnection
                    _pc->SetRemoteDescription(currentSD->Clone(), _setRemoteSdpObserver);
                }
                else {
                    _renegotiate = true;
                    return;
                }
            }
            _pc->CreateDataChannelOrError("", nullptr);
            _pc->CreateOffer(_offerCreationObserver.get(), options);
        }
    }
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (_pc) {
        _pc->CreateAnswer(_answerCreationObserver.get(), options);
    }
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (_pc && desc) {
        _pc->SetLocalDescription(std::move(desc), _setLocalSdpObserver);
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (_pc && desc) {
        _pc->SetRemoteDescription(std::move(desc), _setRemoteSdpObserver);
    }
}

void Transport::setRestartingIce(bool restartingIce)
{
    if (valid() && restartingIce != _restartingIce.exchange(restartingIce)) {
        logVerbose("restarting ICE is " + std::string(restartingIce ? "ON" : "OFF"));
    }
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::state() const
{
    return _pc ? _pc->peer_connection_state() : webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

webrtc::PeerConnectionInterface::IceConnectionState Transport::iceConnectionState() const
{
    return _pc ? _pc->ice_connection_state() : webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed;
}

webrtc::PeerConnectionInterface::SignalingState Transport::signalingState() const
{
    return _pc ? _pc->signaling_state() : webrtc::PeerConnectionInterface::SignalingState::kClosed;
}

std::vector<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> Transport::transceivers() const
{
    if (_pc) {
        return _pc->GetTransceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> Transport::receivers() const
{
    if (_pc) {
        return _pc->GetReceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> Transport::senders() const
{
    if (_pc) {
        return _pc->GetSenders();
    }
    return {};
}

const webrtc::SessionDescriptionInterface* Transport::localDescription() const
{
    return _pc ? _pc->local_description() : nullptr;
}

const webrtc::SessionDescriptionInterface* Transport::remoteDescription() const
{
    return _pc ? _pc->remote_description() : nullptr;
}

bool Transport::iceConnected() const
{
    if (_pc) {
        switch (_pc->ice_connection_state()) {
            case webrtc::PeerConnectionInterface::kIceConnectionConnected:
            case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
                return true;
            default:
                break;
        }
    }
    return false;
}

void Transport::close()
{
    if (_pc) {
        _pc->Close();
    }
}

bool Transport::removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (_pc) {
        const auto error = _pc->RemoveTrackOrError(std::move(sender));
        if (error.ok()) {
            return true;
        }
        logWebRTCError(error);
    }
    return false;
}

bool Transport::addIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    bool added = false;
    if (candidate && _pc) {
        if (_pc->remote_description() && !_restartingIce) {
            added = _pc->AddIceCandidate(candidate);
        }
        else {
            LOCK_WRITE_SAFE_OBJ(_pendingCandidates);
            added = _pendingCandidates->insert(candidate).second;
        }
    }
    return added;
}

rtc::scoped_refptr<webrtc::DataChannelInterface> Transport::
    createDataChannel(const std::string& label,
                      const webrtc::DataChannelInit& init,
                      webrtc::DataChannelObserver* observer)
{
    if (_pc) {
        auto result = _pc->CreateDataChannelOrError(label, &init);
        if (result.ok()) {
            if (observer) {
                result.value()->RegisterObserver(observer);
            }
            return result.MoveValue();
        }
        logWebRTCError(result.error());
    }
    return {};
}

rtc::scoped_refptr<webrtc::RtpTransceiverInterface> Transport::
    addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    if (_pc && track) {
        auto result = _pc->AddTransceiver(std::move(track));
        if (result.ok()) {
            return result.MoveValue();
        }
        logWebRTCError(result.error());
    }
    return {};
}

rtc::scoped_refptr<webrtc::RtpTransceiverInterface> Transport::
    addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                   const webrtc::RtpTransceiverInit& init)
{
    if (_pc && track) {
        auto result = _pc->AddTransceiver(std::move(track), init);
        if (result.ok()) {
            return result.MoveValue();
        }
        logWebRTCError(result.error());
    }
    return {};
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> Transport::
    addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
             const std::vector<std::string>& streamIds,
             const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings)
{
    if (_pc) {
        webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> result;
        if (initSendEncodings.empty()) {
            result = _pc->AddTrack(std::move(track), streamIds);
        }
        else {
            result = _pc->AddTrack(std::move(track), streamIds, initSendEncodings);
        }
        if (result.ok()) {
            return result.MoveValue();
        }
        logWebRTCError(result.error());
    }
    return {};
}

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    if (_pc) {
        auto res = _pc->SetConfiguration(config);
        if (!res.ok()) {
            logWebRTCError(res);
            if (_listener) {
                _listener->onSetConfigurationError(*this, std::move(res));
            }
            return false;
        }
        return true;
    }
    return false;
}

void Transport::logWebRTCError(const webrtc::RTCError& error) const
{
    if (!error.ok()) {
        logError(error.message());
    }
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (_listener) {
        _listener->onSdpCreated(*this, std::move(desc));
    }
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    if (_listener) {
        _listener->onSdpCreationFailure(*this, type, std::move(error));
    }
}

void Transport::onCompleted(bool local)
{
    if (_listener) {
        const auto desc = local ? _pc->local_description() : _pc->remote_description();
        _listener->onSdpSet(*this, local, desc);
    }
    if (!local) {
        {
            LOCK_WRITE_SAFE_OBJ(_pendingCandidates);
            for (const auto candidate : _pendingCandidates.constRef()) {
                _pc->AddIceCandidate(candidate);
            }
            _pendingCandidates->clear();
        }
        setRestartingIce(false);
    }
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    if (_listener) {
        _listener->onSdpSetFailure(*this, local, std::move(error));
    }
}

void Transport::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    if (_listener) {
        _listener->onSignalingChange(*this, newState);
    }
}

void Transport::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    if (_listener) {
        _listener->onAddStream(*this, std::move(stream));
    }
}

void Transport::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    if (_listener) {
        _listener->onRemoveStream(*this, std::move(stream));
    }
}

void Transport::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (_listener) {
        _listener->onDataChannel(*this, std::move(channel));
    }
}

void Transport::OnRenegotiationNeeded()
{
    if (_listener) {
        _listener->onRenegotiationNeeded(*this);
    }
}

void Transport::OnNegotiationNeededEvent(uint32_t eventId)
{
    if (_listener) {
        _listener->onNegotiationNeededEvent(*this, eventId);
    }
}

void Transport::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (_listener) {
        _listener->onIceConnectionChange(*this, newState);
    }
}

void Transport::OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (_listener) {
        _listener->onStandardizedIceConnectionChange(*this, newState);
    }
}

void Transport::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    if (_listener) {
        _listener->onConnectionChange(*this, newState);
    }
}

void Transport::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    if (_listener) {
        _listener->onIceGatheringChange(*this, newState);
    }
}

void Transport::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    if (_listener) {
        _listener->onIceCandidate(*this, candidate);
    }
}

void Transport::OnIceCandidateError(const std::string& address, int port,
                                    const std::string& url,
                                    int errorCode, const std::string& errorText)
{
    if (_listener) {
        _listener->onIceCandidateError(*this, address, port, url, errorCode, errorText);
    }
}

void Transport::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates)
{
    if (_listener) {
        _listener->onIceCandidatesRemoved(*this, candidates);
    }
}

void Transport::OnIceConnectionReceivingChange(bool receiving)
{
    if (_listener) {
        _listener->onIceConnectionReceivingChange(*this, receiving);
    }
}

void Transport::OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event)
{
    if (_listener) {
        _listener->onIceSelectedCandidatePairChanged(*this, event);
    }
}

void Transport::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                           const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    if (_listener) {
        _listener->onAddTrack(*this, std::move(receiver), streams);
    }
}

void Transport::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (_listener) {
        _listener->onTrack(*this, std::move(transceiver));
    }
}

void Transport::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (_listener) {
        _listener->onRemoveTrack(*this, std::move(receiver));
    }
}

void Transport::OnInterestingUsage(int usagePattern)
{
    if (_listener) {
        _listener->onInterestingUsage(*this, usagePattern);
    }
}

std::string_view Transport::logCategory() const
{
    static const std::string_view category("Transport");
    return category;
}

} // namespace LiveKitCpp
