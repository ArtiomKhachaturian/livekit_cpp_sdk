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
#include "TransportImpl.h"
#include "DataChannel.h"
#include "PeerConnectionFactory.h"
#include "TransportListener.h"
#include "RtcUtils.h"
#include "livekit/signaling/sfu/SignalTarget.h"

namespace LiveKitCpp
{

TransportImpl::TransportImpl(SignalTarget target,
                             const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                             TransportListener* listener,
                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                             const std::string& identity,
                             const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<webrtc::PeerConnectionObserver>(logger)
    , _target(target)
    , _logCategory(toString(target) + "_" + identity)
    , _signalingThread(pcf ? pcf->signalingThread() : std::weak_ptr<webrtc::Thread>())
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

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> TransportImpl::peerConnection() const noexcept
{
    if (!closed()) {
        return _pc;
    }
    return {};
}

std::shared_ptr<webrtc::Thread> TransportImpl::signalingThread() const
{
    return _signalingThread.lock();
}

void TransportImpl::close()
{
    if (!_closed.exchange(true)) {
        if (canLogInfo()) {
            logInfo("close peer connection");
        }
        auto self = shared_from_this();
        // force
        OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState::kClosed);
        _listener.reset();
        if (const auto thread = signalingThread()) {
            thread->PostTask([self = std::move(self)]() { self->_pc->Close(); });
        }
        else {
            _pc->Close();
        }
    }
}

void TransportImpl::logWebRTCError(const webrtc::RTCError& error,
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

void TransportImpl::removeTrackBySender(const webrtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    if (sender) {
        const auto id = sender->id();
        const auto type = sender->media_type();
        const auto kind = webrtc::MediaTypeToString(type);
        const auto streamIds = sender->stream_ids();
        const auto res = _pc->RemoveTrackOrError(std::move(sender));
        if (res.ok()) {
            if (canLogVerbose()) {
                logVerbose("local " + kind + " track '" + id + "' track was removed");
            }
            notify(&TransportListener::onLocalTrackRemoved, id, type, streamIds);
        }
        else {
            if (canLogError()) {
                logWebRTCError(res, "failed to remove '" + id + "' local " + kind + " track");
            }
            notify(&TransportListener::onLocalTrackRemoveFailure, id, type, streamIds, std::move(res));
        }
    }
}

webrtc::PeerConnectionInterface::PeerConnectionState TransportImpl::state() const noexcept
{
    if (!closed()) {
        return _pcState;
    }
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

webrtc::PeerConnectionInterface::IceConnectionState TransportImpl::iceConnectionState() const noexcept
{
    return _iceConnState;
}

webrtc::PeerConnectionInterface::SignalingState TransportImpl::signalingState() const noexcept
{
    return _signalingState;
}

webrtc::PeerConnectionInterface::IceGatheringState TransportImpl::iceGatheringState() const noexcept
{
    return _iceGatheringState;
}

void TransportImpl::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState)
{
    if (changeAndLogState(newState, _signalingState)) {
        notify(&TransportListener::onSignalingChange, newState);
    }
}

void TransportImpl::OnAddStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    notify(&TransportListener::onRemoteStreamAdded, std::move(stream));
}

void TransportImpl::OnRemoveStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    notify(&TransportListener::onRemoteStreamRemoved, std::move(stream));
}

void TransportImpl::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    notify(&TransportListener::onRemoteDataChannelOpened,
           DataChannel::create(false, std::move(channel)));
}

void TransportImpl::OnNegotiationNeededEvent(uint32_t eventId)
{
    notify(&TransportListener::onNegotiationNeededEvent, eventId);
}

void TransportImpl::OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState)
{
    if (changeAndLogState(newState, _iceConnState)) {
        notify(&TransportListener::onIceConnectionChange, newState);
    }
}

void TransportImpl::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState)
{
    if (changeAndLogState(newState, _pcState)) {
        notify(&TransportListener::onConnectionChange, newState);
    }
}

void TransportImpl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState)
{
    if (changeAndLogState(newState, _iceGatheringState)) {
        notify(&TransportListener::onIceGatheringChange, newState);
    }
}

void TransportImpl::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    notify(&TransportListener::onIceCandidateGathered, candidate);
}

void TransportImpl::OnIceCandidateError(const std::string& address, int port,
                                          const std::string& url,
                                          int errorCode, const std::string& errorText)
{
    if (canLogError()) {
        logError("a failure occured when gathering ICE candidates: " + errorText);
    }
    notify(&TransportListener::onIceCandidateGatheringError, address, port,
           url, errorCode, errorText);
}

void TransportImpl::OnIceCandidatesRemoved(const std::vector<webrtc::Candidate>& candidates)
{
    notify(&TransportListener::onIceCandidatesRemoved, candidates);
}

void TransportImpl::OnIceConnectionReceivingChange(bool receiving)
{
    notify(&TransportListener::onIceConnectionReceivingChange, receiving);
}

void TransportImpl::OnIceSelectedCandidatePairChanged(const webrtc::CandidatePairChangeEvent& event)
{
    notify(&TransportListener::onIceSelectedCandidatePairChanged, event);
}

void TransportImpl::OnAddTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                 const std::vector<webrtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    notify(&TransportListener::onRemoteTrackAdded, std::move(receiver), streams);
}

/*void TransportImpl::OnTrack(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (transceiver) {
        if (auto receiver = transceiver->receiver()) {
            const auto streams = receiver->streams();
        }
    }
    notify(&TransportListener::onRemoteTrackAdded, std::move(transceiver));
}*/

void TransportImpl::OnRemoveTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    notify(&TransportListener::onRemotedTrackRemoved, std::move(receiver));
}

void TransportImpl::OnInterestingUsage(int usagePattern)
{
    notify(&TransportListener::onInterestingUsage, usagePattern);
}

webrtc::scoped_refptr<webrtc::PeerConnectionInterface> TransportImpl::
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         webrtc::PeerConnectionObserver* observer) const
{
    auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(observer));
    if (pc.ok()) {
        return pc.MoveValue();
    }
    if (canLogError()) {
        logWebRTCError(pc.error(), "unable to create RTC peer connection");
    }
    return {};
}

template <typename TState>
bool TransportImpl::changeAndLogState(TState newState, std::atomic<TState>& val) const
{
    const TState oldState = val.exchange(newState);
    if (oldState != newState) {
        if (canLogVerbose()) {
            logVerbose(makeStateChangesString(oldState, newState));
        }
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
