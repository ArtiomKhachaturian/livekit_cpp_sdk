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

class Transport::Impl : public Bricks::LoggableS<webrtc::PeerConnectionObserver>,
                          public std::enable_shared_from_this<Impl>
{
public:
    Impl(SignalTarget target, const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
         TransportListener* listener, const std::shared_ptr<Bricks::Logger>& logger);
    bool closed() const noexcept { return _closed; }
    SignalTarget target() const { return _target; }
    rtc::Thread* signalingThread() const;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection() const noexcept;
    template <class Method, typename... Args>
    void invoke(const Method& method, Args&&... args) const;
    void resetTransportListener();
    void close();
    void logWebRTCError(const webrtc::RTCError& error, std::string_view prefix = {}) const;
    void removeTrackBySender(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    webrtc::PeerConnectionInterface::IceConnectionState iceConnectionState() const noexcept;
    webrtc::PeerConnectionInterface::SignalingState signalingState() const noexcept;
    webrtc::PeerConnectionInterface::IceGatheringState iceGatheringState() const noexcept;
    // impl. of webrtc::PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) final;
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) final;
    void OnNegotiationNeededEvent(uint32_t eventId) final;
    void OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) final;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) final;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) final;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) final;
    void OnIceCandidateError(const std::string& address, int port, const std::string& url,
                             int errorCode, const std::string& errorText) final;
    void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) final;
    void OnIceConnectionReceivingChange(bool receiving) final;
    void OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) final;
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) final;
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    void OnInterestingUsage(int usagePattern) final;
protected:
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final { return _logCategory; }
private:
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         webrtc::PeerConnectionObserver* observer) const;
    template<typename TState>
    bool changeAndLogState(TState newState, std::atomic<TState>& val) const;
private:
    const SignalTarget _target;
    const std::string _logCategory;
    Bricks::Listener<TransportListener*> _listener;
    std::atomic_bool _closed = false;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _pcState;
    std::atomic<webrtc::PeerConnectionInterface::IceConnectionState> _iceConnState;
    std::atomic<webrtc::PeerConnectionInterface::SignalingState> _signalingState;
    std::atomic<webrtc::PeerConnectionInterface::IceGatheringState> _iceGatheringState;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
};

Transport::Transport(SignalTarget target, TransportListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(new Impl(target, pcf, conf, listener, logger))
{
    if (_impl->peerConnection()) {
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
    return _impl->target();
}

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    if (const auto thread = signalingThread()) {
        _impl->logInfo("request to set peer connection configuration");
        thread->PostTask([config, implRef = weak(_impl)]() {
            if (const auto impl = implRef.lock()) {
                auto res = impl->peerConnection()->SetConfiguration(config);
                if (res.ok()) {
                    impl->logVerbose("set of peer connection configuration successfully completed");
                    impl->invoke(&TransportListener::onConfigurationSet, config);
                }
                else {
                    impl->logWebRTCError(res, "failed to set connection configuration configuration");
                    impl->invoke(&TransportListener::onConfigurationSetFailure, std::move(res));
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
        _impl->logInfo("request to create '" + label + "' data channel");
        thread->PostTask([label, init, implRef = weak(_impl)]() {
            if (const auto impl = implRef.lock()) {
                auto result = impl->peerConnection()->CreateDataChannelOrError(label, &init);
                if (result.ok()) {
                    impl->logVerbose("data channel '" + label + "' has been created");
                    impl->invoke(&TransportListener::onLocalDataChannelCreated,
                                   DataChannel::create(true, result.MoveValue()));
                }
                else {
                    impl->logWebRTCError(result.error(), "unable to create data channel '"
                                           + label + "'");
                    impl->invoke(&TransportListener::onLocalDataChannelCreationFailure,
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
            _impl->logInfo("request to adding local '" + id + "' " + kind + " track");
            thread->PostTask([id, kind, track = std::move(track), streamIds,
                              initSendEncodings, implRef = weak(_impl)]() {
                if (const auto impl = implRef.lock()) {
                    const auto pc = impl->peerConnection();
                    if (!pc) {
                        return;
                    }
                    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>> result;
                    if (initSendEncodings.empty()) {
                        result = pc->AddTrack(std::move(track), streamIds);
                    }
                    else {
                        result = pc->AddTrack(std::move(track), streamIds,
                                              initSendEncodings);
                    }
                    if (result.ok()) {
                        impl->logVerbose("local " + kind + " track '" + id + "' was added");
                        impl->invoke(&TransportListener::onLocalTrackAdded, result.MoveValue());
                    }
                    else {
                        impl->logWebRTCError(result.error(), "failed to add '" +
                                               id + "' local " + kind + " track");
                        impl->invoke(&TransportListener::onLocalTrackAddFailure,
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
            const auto kind = cricket::MediaTypeToString(sender->media_type());
            _impl->logInfo("request to removal '" + id + "' local " + kind + " track");
            thread->PostTask([sender = std::move(sender), implRef = weak(_impl)]() {
                if (const auto impl = implRef.lock()) {
                    impl->removeTrackBySender(sender);
                }
            });
            return true;
        }
    }
    return false;
}

bool Transport::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track) {
        if (const auto thread = signalingThread()) {
            const auto id = track->id();
            const auto kind = track->kind();
            _impl->logInfo("request to removal '" + id + "' local " + kind + " track");
            thread->PostTask([id, kind, track, implRef = weak(_impl)]() {
                if (const auto impl = implRef.lock()) {
                    const auto pc = impl->peerConnection();
                    if (!pc) {
                        return;
                    }
                    rtc::scoped_refptr<webrtc::RtpSenderInterface> trackSender;
                    for (const auto& sender : pc->GetSenders()) {
                        if (sender->track() == track) {
                            trackSender = sender;
                        }
                    }
                    impl->removeTrackBySender(trackSender);
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
        if (const auto& pc = _impl->peerConnection()) {
            const auto info = candidate->candidate().ToSensitiveString();
            _impl->logInfo("request to add (" + info + ") ICE candidate");
            auto handler = [info, implRef = weak(_impl)](webrtc::RTCError error) {
                if (const auto impl = implRef.lock()) {
                    if (error.ok()) {
                        impl->logVerbose("ICE candidate (" + info +
                                           ") has been added successfully");
                        impl->invoke(&TransportListener::onIceCandidateAdded);
                    }
                    else {
                        impl->logWebRTCError(error, "failed to add ICE candidate (" +
                                               info + ")");
                        impl->invoke(&TransportListener::onIceCandidateAddFailure,
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
            _impl->logInfo("request to adding '" + id + "' " + kind + " transceiver");
            thread->PostTask([id, kind, track = std::move(track), init, implRef = weak(_impl)]() {
                if (const auto impl = implRef.lock()) {
                    const auto pc = impl->peerConnection();
                    if (!pc) {
                        return;
                    }
                    auto result = pc->AddTransceiver(std::move(track), init);
                    if (result.ok()) {
                        impl->logVerbose(kind + " transceiver '" + id + "' was added");
                        impl->invoke(&TransportListener::onTransceiverAdded, result.MoveValue());
                    }
                    else {
                        impl->logWebRTCError(result.error(), "failed to add '" +
                                               id + "' " + kind + " transceiver");
                        impl->invoke(&TransportListener::onTransceiverAddFailure,
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
    if (const auto pc = _impl->peerConnection()) {
        _impl->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kOffer));
        pc->CreateOffer(_offerCreationObserver.get(), options); // non-blocking call
    }
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (const auto pc = _impl->peerConnection()) {
        _impl->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kAnswer));
        pc->CreateAnswer(_answerCreationObserver.get(), options); // non-blocking call
    }
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto pc = _impl->peerConnection()) {
            _impl->logInfo("request to set local " + desc->type());
            pc->SetLocalDescription(std::move(desc), _setLocalSdpObserver);
        }
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto pc = _impl->peerConnection()) {
            _impl->logInfo("request to set remote " + desc->type());
            pc->SetRemoteDescription(std::move(desc), _setRemoteSdpObserver);
        }
    }
}

const webrtc::SessionDescriptionInterface* Transport::localDescription() const
{
    if (const auto pc = _impl->peerConnection()) {
        return pc->local_description();
    }
    return nullptr;
}

const webrtc::SessionDescriptionInterface* Transport::remoteDescription() const
{
    if (const auto pc = _impl->peerConnection()) {
        return pc->remote_description();
    }
    return nullptr;
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::state() const noexcept
{
    return _impl->state();
}

webrtc::PeerConnectionInterface::IceConnectionState Transport::iceConnectionState() const noexcept
{
    return _impl->iceConnectionState();
}

webrtc::PeerConnectionInterface::SignalingState Transport::signalingState() const noexcept
{
    return _impl->signalingState();
}

webrtc::PeerConnectionInterface::IceGatheringState Transport::iceGatheringState() const noexcept
{
    return _impl->iceGatheringState();
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
    if (const auto pc = _impl->peerConnection()) {
        return pc->GetTransceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> Transport::receivers() const
{
    if (const auto pc = _impl->peerConnection()) {
        return pc->GetReceivers();
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> Transport::senders() const
{
    if (const auto pc = _impl->peerConnection()) {
        return pc->GetSenders();
    }
    return {};
}

bool Transport::valid() const
{
    return nullptr != _impl->peerConnection();
}

void Transport::close()
{
    if (!closed()) {
        _offerCreationObserver->setListener(nullptr);
        _answerCreationObserver->setListener(nullptr);
        _setLocalSdpObserver->setListener(nullptr);
        _setRemoteSdpObserver->setListener(nullptr);
        _impl->close();
        _impl->resetTransportListener();
    }
}

rtc::Thread* Transport::signalingThread() const
{
    return _impl->signalingThread();
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        _impl->logVerbose(desc->type() + " created successfully");
    }
    _impl->invoke(&TransportListener::onSdpCreated, std::move(desc));
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    _impl->logWebRTCError(error, "failed to create " + sdpTypeToString(type));
    _impl->invoke(&TransportListener::onSdpCreationFailure, type, std::move(error));
}

void Transport::onCompleted(bool local)
{
    if (const auto pc = _impl->peerConnection()) {
        if (const auto desc = local ? pc->local_description() : pc->remote_description()) {
            _impl->logVerbose(std::string(local ? "local " : "remote ") +
                                desc->type() + " has been set successfully");
            _impl->invoke(&TransportListener::onSdpSet, local, desc);
        }
    }
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    _impl->logWebRTCError(error, "failed to set " + std::string(local ? "local SDP" : "remote SDP"));
    _impl->invoke(&TransportListener::onSdpSetFailure, local, std::move(error));
}

Transport::Impl::Impl(SignalTarget target,
                      const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                      const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                      TransportListener* listener,
                      const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<webrtc::PeerConnectionObserver>(logger)
    , _target(target)
    , _logCategory(toString(target))
    , _pcState(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
    , _iceConnState(webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew)
    , _signalingState(webrtc::PeerConnectionInterface::SignalingState::kStable)
    , _iceGatheringState(webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew)
    , _pc(createPeerConnection(pcf, conf, this))
{
    if (_pc) {
        _listener.set(listener);
    }
    else {
        _closed = true;
    }
}

rtc::Thread* Transport::Impl::signalingThread() const
{
    return _pc ? _pc->signaling_thread() : nullptr;
}

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> Transport::Impl::peerConnection() const noexcept
{
    if (!closed()) {
        return _pc;
    }
    return {};
}

template <class Method, typename... Args>
void Transport::Impl::invoke(const Method& method, Args&&... args) const
{
    _listener.invoke(method, _target, std::forward<Args>(args)...);
}

void Transport::Impl::resetTransportListener()
{
    // force
    OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState::kClosed);
    _listener.reset();
}

void Transport::Impl::close()
{
    if (!_closed.exchange(true)) {
        logInfo("close peer connection");
        if (const auto thread = signalingThread()) {
            thread->PostTask([self = shared_from_this()]() {
                self->_pc->Close();
            });
        }
        else {
            _pc->Close();
        }
    }
}

void Transport::Impl::logWebRTCError(const webrtc::RTCError& error,
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

void Transport::Impl::removeTrackBySender(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    if (sender) {
        const auto id = sender->id();
        const auto type = sender->media_type();
        const auto kind = cricket::MediaTypeToString(type);
        const auto streamIds = sender->stream_ids();
        const auto res = _pc->RemoveTrackOrError(std::move(sender));
        if (res.ok()) {
            logVerbose("local " + kind + " track '" + id + "' track was removed");
            invoke(&TransportListener::onLocalTrackRemoved, id, type, streamIds);
        }
        else {
            logWebRTCError(res, "failed to remove '" + id + "' local " + kind + " track");
            invoke(&TransportListener::onLocalTrackRemoveFailure, id, type, streamIds, std::move(res));
        }
    }
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::Impl::state() const noexcept
{
    if (!closed()) {
        return _pcState;
    }
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

webrtc::PeerConnectionInterface::IceConnectionState Transport::Impl::iceConnectionState() const noexcept
{
    return _iceConnState;
}

webrtc::PeerConnectionInterface::SignalingState Transport::Impl::signalingState() const noexcept
{
    return _signalingState;
}

webrtc::PeerConnectionInterface::IceGatheringState Transport::Impl::iceGatheringState() const noexcept
{
    return _iceGatheringState;
}

void Transport::Impl::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    if (changeAndLogState(newState, _signalingState)) {
        invoke(&TransportListener::onSignalingChange, newState);
    }
}

void Transport::Impl::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    invoke(&TransportListener::onRemoteStreamAdded, std::move(stream));
}

void Transport::Impl::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    invoke(&TransportListener::onRemoteStreamRemoved, std::move(stream));
}

void Transport::Impl::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    invoke(&TransportListener::onRemoteDataChannelOpened,
           DataChannel::create(false, std::move(channel)));
}

void Transport::Impl::OnNegotiationNeededEvent(uint32_t eventId)
{
    invoke(&TransportListener::onNegotiationNeededEvent, eventId);
}

void Transport::Impl::OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (changeAndLogState(newState, _iceConnState)) {
        invoke(&TransportListener::onIceConnectionChange, newState);
    }
}

void Transport::Impl::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    if (changeAndLogState(newState, _pcState)) {
        invoke(&TransportListener::onConnectionChange, newState);
    }
}

void Transport::Impl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    if (changeAndLogState(newState, _iceGatheringState)) {
        invoke(&TransportListener::onIceGatheringChange, newState);
    }
}

void Transport::Impl::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    invoke(&TransportListener::onIceCandidateGathered, candidate);
}

void Transport::Impl::OnIceCandidateError(const std::string& address, int port,
                                          const std::string& url,
                                          int errorCode, const std::string& errorText)
{
    logError("a failure occured when gathering ICE candidates: " + errorText);
    invoke(&TransportListener::onIceCandidateGatheringError, address, port,
           url, errorCode, errorText);
}

void Transport::Impl::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates)
{
    invoke(&TransportListener::onIceCandidatesRemoved, candidates);
}

void Transport::Impl::OnIceConnectionReceivingChange(bool receiving)
{
    invoke(&TransportListener::onIceConnectionReceivingChange, receiving);
}

void Transport::Impl::OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event)
{
    invoke(&TransportListener::onIceSelectedCandidatePairChanged, event);
}

void Transport::Impl::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                           const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    invoke(&TransportListener::onTrackAdded, std::move(receiver), streams);
}

void Transport::Impl::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    invoke(&TransportListener::onRemoteTrackAdded, std::move(transceiver));
}

void Transport::Impl::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    invoke(&TransportListener::onRemotedTrackRemoved, std::move(receiver));
}

void Transport::Impl::OnInterestingUsage(int usagePattern)
{
    invoke(&TransportListener::onInterestingUsage, usagePattern);
}

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> Transport::Impl::
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

template<typename TState>
bool Transport::Impl::changeAndLogState(TState newState, std::atomic<TState>& val) const
{
    const TState oldState = val.exchange(newState);
    if (oldState != newState) {
        logVerbose(makeStateChangesString(oldState, newState));
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
