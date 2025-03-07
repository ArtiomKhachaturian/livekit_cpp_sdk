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
#include "TransportManager.h"
#include "TransportManagerListener.h"
#include "PeerConnectionFactory.h"

namespace {

inline auto mapStates(const std::vector<const LiveKitCpp::Transport*>& transports) {
    std::vector<webrtc::PeerConnectionInterface::PeerConnectionState> res;
    res.reserve(transports.size());
    for (const auto transport : transports) {
        if (transport) {
            res.push_back(transport->state());
        }
    }
    return res;
}

template<typename T>
inline bool every(const T& required, const std::vector<T>& vals) {
    if (!vals.empty()) {
        for (const auto& val : vals) {
            if (val != required) {
                return false;
            }
        }
        return true;
    }
    return false;
}

}

namespace LiveKitCpp
{

TransportManager::TransportManager(bool subscriberPrimary,
                                   TransportManagerListener* listener,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
    : Bricks::LoggableS<TransportListener>(pcf ? pcf->logger() : std::shared_ptr<Bricks::Logger>())
    , _listener(listener)
    , _publisher(!subscriberPrimary, SignalTarget::Publisher, this, pcf, conf)
    , _subscriber(subscriberPrimary, SignalTarget::Subscriber, this, pcf, conf)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
{
    _publisherConnectionRequired = !subscriberPrimary;
    _subscriberConnectionRequired = subscriberPrimary;
}

TransportManager::~TransportManager()
{
    _subscriber.close();
    _publisher.close();
}

bool TransportManager::valid() const noexcept
{
    return _subscriber.valid() && _publisher.valid();
}

void TransportManager::requirePublisher(bool require)
{
    if (require != _publisherConnectionRequired.exchange(require)) {
        updateState();
    }
}

void TransportManager::requireSubscriber(bool require)
{
    if (require != _subscriberConnectionRequired.exchange(require)) {
        updateState();
    }
}

bool TransportManager::addIceCandidate(const webrtc::IceCandidateInterface* candidate,
                                       SignalTarget target)
{
    if (candidate) {
        switch (target) {
            case SignalTarget::Publisher:
                return _publisher.addIceCandidate(candidate);
            case SignalTarget::Subscriber:
                return _subscriber.addIceCandidate(candidate);
            default:
                break;
        }
    }
    return false;
}

void TransportManager::createAndSetPublisherOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    _publisher.createOffer(options);
}

void TransportManager::createSubscriberAnswerFromOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        _subscriber.setRemoteDescription(std::move(desc));
    }
}

void TransportManager::setPublisherAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    _publisher.setRemoteDescription(std::move(desc));
}

bool TransportManager::updateConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config,
                                           bool iceRestart)
{
    if (_subscriber.setConfiguration(config) && _publisher.setConfiguration(config)) {
        if (iceRestart) {
            triggerIceRestart();
        }
        return true;
    }
    return false;
}

void TransportManager::close()
{
    if (webrtc::PeerConnectionInterface::SignalingState::kClosed != _publisher.signalingState()) {
        for (auto&& sender : _publisher.senders()) {
            _publisher.removeTrack(sender);
        }
    }
    _publisher.close();
    _subscriber.close();
    updateState();
}

void TransportManager::triggerIceRestart()
{
    _subscriber.setRestartingIce(true);
    if (needsPublisher()) {
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
        options.ice_restart = true;
        createAndSetPublisherOffer(options);
    }
}

rtc::scoped_refptr<webrtc::DataChannelInterface> TransportManager::
    createPublisherDataChannel(const std::string& label,
                               const webrtc::DataChannelInit& init,
                               webrtc::DataChannelObserver* observer)
{
    return _publisher.createDataChannel(label, init, observer);
}

rtc::scoped_refptr<webrtc::RtpTransceiverInterface> TransportManager::
    addPublisherTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    return _publisher.addTransceiver(std::move(track));
}

rtc::scoped_refptr<webrtc::RtpTransceiverInterface> TransportManager::
    addPublisherTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                            const webrtc::RtpTransceiverInit& init)
{
    return _publisher.addTransceiver(std::move(track), init);
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> TransportManager::
    addPublisherTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                      const std::vector<std::string>& streamIds,
                      const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings)
{
    return _publisher.addTrack(std::move(track), streamIds, initSendEncodings);
}

void TransportManager::updateState()
{
    auto newState = state();
    const auto states = mapStates(requiredTransports());
    if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kConnected, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kConnected;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kFailed, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kFailed;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kClosed, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kNew, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kNew;
    }
    const auto oldState = _state.exchange(newState);
    if (oldState != newState) {
        // TODO: log changes
        /*this.log.debug(
                `pc state change: from ${PCTransportState[previousState]} to ${
                  PCTransportState[this.state]
                }`,
                this.logContext,
              );*/
        if (_listener) {
            _listener->onStateChange(*this, newState, _publisher.state(), _subscriber.state());
        }
    }
}

std::vector<const Transport*> TransportManager::requiredTransports() const
{
    std::vector<const Transport*> transports;
    transports.reserve(2U);
    if (_publisher.valid() && needsPublisher()) {
        transports.push_back(&_publisher);
    }
    if (_subscriber.valid() && needsSubscriber()) {
        transports.push_back(&_subscriber);
    }
    return transports;
}

void TransportManager::onSdpCreated(Transport& transport,
                                    std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    transport.setLocalDescription(std::move(desc));
}

void TransportManager::onSdpCreationFailure(Transport& transport, webrtc::SdpType type,
                                            webrtc::RTCError error)
{
}

void TransportManager::onSdpSet(Transport& transport, bool local,
                                const webrtc::SessionDescriptionInterface* desc)
{
    if (SignalTarget::Publisher == transport.target()) {
        if (local && _listener) {
            _listener->onPublisherOffer(*this, desc);
        }
    }
    else { // subscriber
        if (local) {
            if (_listener) {
                _listener->onSubscriberAnswer(*this, desc);
            }
        }
        else {
            _subscriber.createAnswer();
        }
    }
}

void TransportManager::onSdpSetFailure(Transport& transport, bool local, webrtc::RTCError error)
{
    
}

void TransportManager::onSetConfigurationError(Transport& transport, webrtc::RTCError error)
{
    
}

void TransportManager::onConnectionChange(Transport&, webrtc::PeerConnectionInterface::PeerConnectionState)
{
    updateState();
}

void TransportManager::onIceConnectionChange(Transport&, webrtc::PeerConnectionInterface::IceConnectionState)
{
    updateState();
}

void TransportManager::onSignalingChange(Transport&, webrtc::PeerConnectionInterface::SignalingState)
{
    updateState();
}

void TransportManager::onDataChannel(Transport& transport, rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (_listener && SignalTarget::Subscriber == transport.target()) {
        // in subscriber primary mode, server side opens sub data channels
        _listener->onDataChannel(*this, std::move(channel));
    }
}

void TransportManager::onIceCandidate(Transport& transport, const webrtc::IceCandidateInterface* candidate)
{
    if (candidate && _listener) {
        _listener->onIceCandidate(*this, transport.target(), candidate);
    }
}

void TransportManager::onTrack(Transport& transport, rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (_listener && SignalTarget::Subscriber == transport.target()) {
        _listener->onRemoteTrack(*this, std::move(transceiver));
    }
}

} // namespace LiveKitCpp
