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
#include "RoomUtils.h"
#include "Utils.h"

// https://github.com/livekit/client-sdk-js/blob/main/src/room/PCTransport.ts

namespace LiveKitCpp
{

Transport::Transport(SignalTarget target, TransportListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<CreateSdpListener, SetSdpListener, webrtc::PeerConnectionObserver>(logger)
    , _logCategory(std::string(toString(target)))
    , _target(target)
    , _listener(listener)
    , _pc(createPeerConnection(pcf, conf))
    , _pcState(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
    , _iceConnState(webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew)
    , _signalingState(webrtc::PeerConnectionInterface::SignalingState::kStable)
    , _iceGatheringState(webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew)
{
    if (_pc) {
        _offerCreationObserver = webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kOffer);
        _answerCreationObserver = webrtc::make_ref_counted<CreateSdpObserver>(webrtc::SdpType::kAnswer);
        _setLocalSdpObserver = webrtc::make_ref_counted<SetLocalSdpObserver>();
        _setRemoteSdpObserver = webrtc::make_ref_counted<SetRemoteSdpObserver>();
        _offerCreationObserver->setListener(this);
        _answerCreationObserver->setListener(this);
        _setLocalSdpObserver->setListener(this);
        _setRemoteSdpObserver->setListener(this);
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
    close();
}

void Transport::createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (_pc) {
        logInfo("attempt to create " + sdpTypeToString(webrtc::SdpType::kOffer));
        _pc->CreateOffer(_offerCreationObserver.get(), options);
    }
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (_pc) {
        logInfo("attempt to create " + sdpTypeToString(webrtc::SdpType::kAnswer));
        _pc->CreateAnswer(_answerCreationObserver.get(), options);
    }
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (_pc && desc) {
        logInfo("attempt to set local " + desc->type());
        _pc->SetLocalDescription(std::move(desc), _setLocalSdpObserver);
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (_pc && desc) {
        logInfo("attempt to set remote " + desc->type());
        _pc->SetRemoteDescription(std::move(desc), _setRemoteSdpObserver);
    }
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::state() const noexcept
{
    return _pc ? _pcState.load() : webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

webrtc::PeerConnectionInterface::IceConnectionState Transport::iceConnectionState() const noexcept
{
    return _iceConnState;
}

webrtc::PeerConnectionInterface::SignalingState Transport::signalingState() const noexcept
{
    return _signalingState;
}

webrtc::PeerConnectionInterface::IceGatheringState Transport::iceGatheringState() const noexcept
{
    return _iceGatheringState;
}

bool Transport::iceConnected() const noexcept
{
    switch (iceConnectionState()) {
        case webrtc::PeerConnectionInterface::kIceConnectionConnected:
        case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
            return true;
        default:
            break;
    }
    return false;
}

bool Transport::closed() const noexcept
{
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed == state();
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

void Transport::close()
{
    if (_pc && !closed()) {
        if (canLogInfo()) {
            logInfo("close peer");
        }
        _pc->Close();
    }
}

bool Transport::removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (_pc) {
        const auto error = _pc->RemoveTrackOrError(std::move(sender));
        if (!error.ok()) {
            logWebRTCError(error, "failed to remove local media track");
        }
        return error.ok();
    }
    return false;
}

void Transport::addRemoteIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    if (candidate && _pc) {
        _pc->AddIceCandidate(std::move(candidate), [this](webrtc::RTCError error) {
            if (error.ok()) {
                logVerbose("remote candidate for the ICE agent has been set successfully");
            }
            else {
                logWebRTCError(error, "failed to add remote candidate for the ICE agent");
                if (_listener) {
                    _listener->onRemoteIceCandidateAddFailed(*this, std::move(error));
                }
            }
        });
    }
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
        logWebRTCError(result.error(), "unable to create data channel '" + label + "'");
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
        logWebRTCError(result.error(), "unable to add transceiver");
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
        logWebRTCError(result.error(), "unable to add transceiver");
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
        logWebRTCError(result.error(), "failed to add local media track");
    }
    return {};
}

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    if (_pc) {
        auto res = _pc->SetConfiguration(config);
        if (!res.ok()) {
            logWebRTCError(res, "failed to set RTC configuration");
            if (_listener) {
                _listener->onSetConfigurationError(*this, std::move(res));
            }
            return false;
        }
        return true;
    }
    return false;
}

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> Transport::
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
{
    if (pcf) {
        auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(this));
        if (pc.ok()) {
            return pc.MoveValue();
        }
        logWebRTCError(pc.error(), "unable to create RTC peer connection");
    }
    return {};
}

void Transport::logWebRTCError(const webrtc::RTCError& error,
                               std::string_view prefix) const
{
    if (!error.ok() && canLogError()) {
        if (prefix.empty()) {
            logError(error.message());
        }
        else {
            logError(std::string(prefix) + std::string(": ") + error.message());
        }
    }
}

template<typename TState>
bool Transport::changeAndLogState(TState newState, std::atomic<TState>& holder) const
{
    const TState oldState = holder.exchange(newState);
    if (oldState != newState) {
        logVerbose(makeStateChangesString(oldState, newState));
        return true;
    }
    return false;
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        logInfo(desc->type() + " created successfully");
    }
    if (_listener) {
        _listener->onSdpCreated(*this, std::move(desc));
    }
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    logWebRTCError(error, "failed to create " + sdpTypeToString(type));
    if (_listener) {
        _listener->onSdpCreationFailure(*this, type, std::move(error));
    }
}

void Transport::onCompleted(bool local)
{
    if (const auto desc = local ? _pc->local_description() : _pc->remote_description()) {
        logVerbose(std::string(local ? "local " : "remote ") +
                   desc->type() + " has been set successfully");
        if (_listener) {
            _listener->onSdpSet(*this, local, desc);
        }
    }
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    logWebRTCError(error, "failed to set " + std::string(local ? "local SDP" : "remote SDP"));
    if (_listener) {
        _listener->onSdpSetFailure(*this, local, std::move(error));
    }
}

void Transport::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    if (changeAndLogState(newState, _signalingState) && _listener) {
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

void Transport::OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (changeAndLogState(newState, _iceConnState) && _listener) {
        _listener->onIceConnectionChange(*this, newState);
    }
}

void Transport::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    if (changeAndLogState(newState, _pcState) && _listener) {
        _listener->onConnectionChange(*this, newState);
    }
}

void Transport::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    if (changeAndLogState(newState, _iceGatheringState) && _listener) {
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
    logError("a failure occured when gathering ICE candidates: " + errorText);
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

} // namespace LiveKitCpp
