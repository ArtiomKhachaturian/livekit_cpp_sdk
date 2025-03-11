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
#include "Loggable.h"
#include "DataChannel.h"
#include "TransportListener.h"
#include "CreateSdpObserver.h"
#include "SetSdpObservers.h"
#include "PeerConnectionFactory.h"
#include "RoomUtils.h"
#include "Listener.h"
#include "Utils.h"

namespace {

template<typename T>
inline auto weak(const std::shared_ptr<T>& strong) {
    return std::weak_ptr<T>(strong);
}

inline cricket::MediaType fromString(const std::string& type) {
    if (type == cricket::MediaTypeToString(cricket::MEDIA_TYPE_AUDIO)) {
        return cricket::MEDIA_TYPE_AUDIO;
    }
    if (type == cricket::MediaTypeToString(cricket::MEDIA_TYPE_VIDEO)) {
        return cricket::MEDIA_TYPE_VIDEO;
    }
    if (type == cricket::MediaTypeToString(cricket::MEDIA_TYPE_DATA)) {
        return cricket::MEDIA_TYPE_DATA;
    }
    return cricket::MEDIA_TYPE_UNSUPPORTED;
}

}

// https://github.com/livekit/client-sdk-js/blob/main/src/room/PCTransport.ts
namespace LiveKitCpp
{

class Transport::Holder : public Bricks::LoggableS<>
{
public:
    Holder(SignalTarget target,
           const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
           const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
           webrtc::PeerConnectionObserver* observer,
           TransportListener* listener,
           const std::shared_ptr<Bricks::Logger>& logger);
    SignalTarget target() const { return _target; }
    const auto& peerConnection() const noexcept { return _pc; }
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const;
    void reset() { _listener.reset(); }
    void logWebRTCError(const webrtc::RTCError& error, std::string_view prefix = {}) const;
protected:
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final { return _logCategory; }
private:
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         webrtc::PeerConnectionObserver* observer) const;
private:
    const SignalTarget _target;
    const std::string _logCategory;
    const webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
    Bricks::Listener<TransportListener*> _listener;
};

Transport::Transport(SignalTarget target, TransportListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : _pcf(pcf)
    , _holder(new Holder(target, _pcf, conf, this, listener, logger))
    , _pcState(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
    , _iceConnState(webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew)
    , _signalingState(webrtc::PeerConnectionInterface::SignalingState::kStable)
    , _iceGatheringState(webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew)
{
    if (_holder->peerConnection()) {
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
    close();
}

SignalTarget Transport::target() const noexcept
{
    return _holder->target();
}

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    if (const auto thread = signalingThread()) {
        _holder->logInfo("request to set peer connection configuration");
        thread->PostTask([config, ref = weak(_holder)]() {
            if (const auto holder = ref.lock()) {
                auto res = holder->peerConnection()->SetConfiguration(config);
                if (res.ok()) {
                    holder->logVerbose("set of peer connection configuration successfully completed");
                    holder->invoke(&TransportListener::onConfigurationSet, config);
                }
                else {
                    holder->logWebRTCError(res, "failed to set connection configuration configuration");
                    holder->invoke(&TransportListener::onConfigurationSetFailure, std::move(res));
                }
            }
        });
        return true;
    }
    return false;
}

bool Transport::createDataChannel(const std::string& label, const webrtc::DataChannelInit& init)
{
    if (const auto thread = signalingThread()) {
        _holder->logInfo("request to create '" + label + "' data channel");
        thread->PostTask([label, init, ref = weak(_holder)]() {
            if (const auto holder = ref.lock()) {
                auto result = holder->peerConnection()->CreateDataChannelOrError(label, &init);
                if (result.ok()) {
                    holder->logVerbose("data channel '" + label + "' has been created");
                    holder->invoke(&TransportListener::onLocalDataChannelCreated,
                                   DataChannel::create(true, result.MoveValue()));
                }
                else {
                    holder->logWebRTCError(result.error(), "unable to create data channel '"
                                           + label + "'");
                    holder->invoke(&TransportListener::onLocalDataChannelCreationFailure,
                                   label, init, result.MoveError());
                }
            }
        });
        return true;
    }
    return false;
}

bool Transport::addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                         const std::vector<std::string>& streamIds,
                         const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings)
{
    if (track) {
        if (const auto thread = signalingThread()) {
            const auto id = track->id();
            const auto kind = track->kind();
            _holder->logInfo("request to adding local '" + id + "' " + kind + " track");
            thread->PostTask([id, kind, track = std::move(track), streamIds,
                              initSendEncodings, ref = weak(_holder)]() {
                if (const auto holder = ref.lock()) {
                    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> result;
                    if (initSendEncodings.empty()) {
                        result = holder->peerConnection()->AddTrack(std::move(track),
                                                                    streamIds);
                    }
                    else {
                        result = holder->peerConnection()->AddTrack(std::move(track),
                                                                    streamIds,
                                                                    initSendEncodings);
                    }
                    if (result.ok()) {
                        holder->logVerbose("local " + kind + " track '" + id + "' was added");
                        holder->invoke(&TransportListener::onLocalTrackAdded, result.MoveValue());
                    }
                    else {
                        holder->logWebRTCError(result.error(), "failed to add '" +
                                               id + "' local " + kind + " track");
                        holder->invoke(&TransportListener::onLocalTrackAddFailure,
                                       id, fromString(kind), streamIds, result.MoveError());
                    }
                }
            });
            return true;
        }
    }
    return false;
}

bool Transport::removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender) {
        if (const auto thread = signalingThread()) {
            const auto id = sender->id();
            const auto type = sender->media_type();
            const auto kind = cricket::MediaTypeToString(type);
            _holder->logInfo("request to removal '" + id + "' local " + kind + " track");
            thread->PostTask([id, type, kind, sender = std::move(sender), ref = weak(_holder)]() {
                if (const auto holder = ref.lock()) {
                    const auto streamIds = sender->stream_ids();
                    const auto res = holder->peerConnection()->RemoveTrackOrError(std::move(sender));
                    if (res.ok()) {
                        holder->logVerbose("local " + kind + " track '" + id + "' track was removed");
                        holder->invoke(&TransportListener::onLocalTrackRemoved, id, type, streamIds);
                    }
                    else {
                        holder->logWebRTCError(res, "failed to remove '" +
                                               id + "' local " + kind + " track");
                        holder->invoke(&TransportListener::onLocalTrackRemoveFailure,
                                       id, type, streamIds, std::move(res));
                    }
                }
            });
            return true;
        }
    }
    return false;
}

bool Transport::addIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    if (candidate) {
        if (const auto& pc = _holder->peerConnection()) {
            const auto info = candidate->candidate().ToSensitiveString();
            _holder->logInfo("request to add (" + info + ") ICE candidate");
            auto handler = [info, ref = weak(_holder)](webrtc::RTCError error) {
                if (const auto holder = ref.lock()) {
                    if (error.ok()) {
                        holder->logVerbose("ICE candidate (" + info +
                                           ") has been added successfully");
                        holder->invoke(&TransportListener::onIceCandidateAdded);
                    }
                    else {
                        holder->logWebRTCError(error, "failed to add ICE candidate (" +
                                               info + ")");
                        holder->invoke(&TransportListener::onIceCandidateAddFailure,
                                       std::move(error));
                    }
                }
            };
            pc->AddIceCandidate(std::move(candidate), std::move(handler));
            return true;
        }
    }
    return false;
}

bool Transport::addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                               const webrtc::RtpTransceiverInit& init)
{
    if (track) {
        if (const auto thread = signalingThread()) {
            const auto id = track->id();
            const auto kind = track->kind();
            _holder->logInfo("request to adding '" + id + "' " + kind + " transceiver");
            thread->PostTask([id, kind, track = std::move(track), init, ref = weak(_holder)]() {
                if (const auto holder = ref.lock()) {
                    auto result = holder->peerConnection()->AddTransceiver(std::move(track), init);
                    if (result.ok()) {
                        holder->logVerbose(kind + " transceiver '" + id + "' was added");
                        holder->invoke(&TransportListener::onTransceiverAdded, result.MoveValue());
                    }
                    else {
                        holder->logWebRTCError(result.error(), "failed to add '" +
                                               id + "' " + kind + " transceiver");
                        holder->invoke(&TransportListener::onTransceiverAddFailure,
                                       id, fromString(kind), init, result.MoveError());
                    }
                }
            });
            return true;
        }
    }
    return false;
}


void Transport::createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (const auto& pc = _holder->peerConnection()) {
        _holder->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kOffer));
        pc->CreateOffer(_offerCreationObserver.get(), options); // non-blocking call
    }
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (const auto& pc = _holder->peerConnection()) {
        _holder->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kAnswer));
        pc->CreateAnswer(_answerCreationObserver.get(), options); // non-blocking call
    }
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto& pc = _holder->peerConnection()) {
            _holder->logInfo("request to set local " + desc->type());
            pc->SetLocalDescription(std::move(desc), _setLocalSdpObserver);
        }
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto& pc = _holder->peerConnection()) {
            _holder->logInfo("request to set remote " + desc->type());
            pc->SetRemoteDescription(std::move(desc), _setRemoteSdpObserver);
        }
    }
}

const webrtc::SessionDescriptionInterface* Transport::localDescription() const
{
    if (const auto& pc = _holder->peerConnection()) {
        return pc->local_description();
    }
    return nullptr;
}

const webrtc::SessionDescriptionInterface* Transport::remoteDescription() const
{
    if (const auto& pc = _holder->peerConnection()) {
        return pc->remote_description();
    }
    return nullptr;
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::state() const noexcept
{
    if (_holder->peerConnection()) {
        return _pcState.load();
    }
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
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
    if (const auto& pc = _holder->peerConnection()) {
        return pc->GetTransceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> Transport::receivers() const
{
    if (const auto& pc = _holder->peerConnection()) {
        return pc->GetReceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> Transport::senders() const
{
    if (const auto& pc = _holder->peerConnection()) {
        return pc->GetSenders();
    }
    return {};
}

bool Transport::valid() const
{
    return nullptr != _holder->peerConnection();
}

void Transport::close()
{
    if (const auto& pc = _holder->peerConnection()) {
        _holder->logInfo("close peer connection");
        pc->Close();
        _offerCreationObserver->setListener(nullptr);
        _answerCreationObserver->setListener(nullptr);
        _setLocalSdpObserver->setListener(nullptr);
        _setRemoteSdpObserver->setListener(nullptr);
        _holder->reset();
    }
}

template<typename TState>
bool Transport::changeAndLogState(TState newState, std::atomic<TState>& holder) const
{
    const TState oldState = holder.exchange(newState);
    if (oldState != newState) {
        _holder->logVerbose(makeStateChangesString(oldState, newState));
        return true;
    }
    return false;
}

rtc::Thread* Transport::signalingThread() const noexcept
{
    if (_pcf && _holder->peerConnection()) {
        return _pcf->signalingThread();
    }
    return nullptr;
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        _holder->logVerbose(desc->type() + " created successfully");
    }
    _holder->invoke(&TransportListener::onSdpCreated, std::move(desc));
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    _holder->logWebRTCError(error, "failed to create " + sdpTypeToString(type));
    _holder->invoke(&TransportListener::onSdpCreationFailure, type, std::move(error));
}

void Transport::onCompleted(bool local)
{
    if (const auto& pc = _holder->peerConnection()) {
        if (const auto desc = local ? pc->local_description() : pc->remote_description()) {
            _holder->logVerbose(std::string(local ? "local " : "remote ") +
                                desc->type() + " has been set successfully");
            _holder->invoke(&TransportListener::onSdpSet, local, desc);
        }
    }
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    _holder->logWebRTCError(error, "failed to set " + std::string(local ? "local SDP" : "remote SDP"));
    _holder->invoke(&TransportListener::onSdpSetFailure, local, std::move(error));
}

void Transport::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    if (changeAndLogState(newState, _signalingState)) {
        _holder->invoke(&TransportListener::onSignalingChange, newState);
    }
}

void Transport::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    _holder->invoke(&TransportListener::onRemoteStreamAdded, std::move(stream));
}

void Transport::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    _holder->invoke(&TransportListener::onRemoteStreamRemoved, std::move(stream));
}

void Transport::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    _holder->invoke(&TransportListener::onRemoteDataChannelOpened,
                    DataChannel::create(false, std::move(channel)));
}

void Transport::OnNegotiationNeededEvent(uint32_t eventId)
{
    _holder->invoke(&TransportListener::onNegotiationNeededEvent, eventId);
}

void Transport::OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (changeAndLogState(newState, _iceConnState)) {
        _holder->invoke(&TransportListener::onIceConnectionChange, newState);
    }
}

void Transport::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    if (changeAndLogState(newState, _pcState)) {
        _holder->invoke(&TransportListener::onConnectionChange, newState);
    }
}

void Transport::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    if (changeAndLogState(newState, _iceGatheringState)) {
        _holder->invoke(&TransportListener::onIceGatheringChange, newState);
    }
}

void Transport::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    _holder->invoke(&TransportListener::onIceCandidateGathered, candidate);
}

void Transport::OnIceCandidateError(const std::string& address, int port,
                                    const std::string& url,
                                    int errorCode, const std::string& errorText)
{
    _holder->logError("a failure occured when gathering ICE candidates: " + errorText);
    _holder->invoke(&TransportListener::onIceCandidateGatheringError, address, port,
                    url, errorCode, errorText);
}

void Transport::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates)
{
    _holder->invoke(&TransportListener::onIceCandidatesRemoved, candidates);
}

void Transport::OnIceConnectionReceivingChange(bool receiving)
{
    _holder->invoke(&TransportListener::onIceConnectionReceivingChange, receiving);
}

void Transport::OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event)
{
    _holder->invoke(&TransportListener::onIceSelectedCandidatePairChanged, event);
}

void Transport::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                           const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    _holder->invoke(&TransportListener::onTrackAdded, std::move(receiver), streams);
}

void Transport::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    _holder->invoke(&TransportListener::onRemoteTrackAdded, std::move(transceiver));
}

void Transport::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    _holder->invoke(&TransportListener::onRemotedTrackRemoved, std::move(receiver));
}

void Transport::OnInterestingUsage(int usagePattern)
{
    _holder->invoke(&TransportListener::onInterestingUsage, usagePattern);
}

Transport::Holder::Holder(SignalTarget target,
                          const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                          const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                          webrtc::PeerConnectionObserver* observer,
                          TransportListener* listener,
                          const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _target(target)
    , _logCategory(toString(target))
    , _pc(createPeerConnection(pcf, conf, observer))
    , _listener(listener)
{
}

template <class Method, typename... Args>
void Transport::Holder::invoke(const Method& method, Args&&... args) const
{
    _listener.invoke(method, _target, std::forward<Args>(args)...);
}

void Transport::Holder::logWebRTCError(const webrtc::RTCError& error,
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

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> Transport::Holder::
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         webrtc::PeerConnectionObserver* observer) const
{
    auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(observer));
    if (pc.ok()) {
        return pc.MoveValue();
    }
    logWebRTCError(pc.error(), "unable to create RTC peer connection");
    return {};
}

} // namespace LiveKitCpp
