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
#include "CreateSdpObserver.h"
#include "DataChannel.h"
#include "Logger.h"
#include "Loggable.h"
#include "TransportImpl.h"
#include "TransportListener.h"
#include "SdpPatch.h"
#include "SetSdpObservers.h"
#include "PeerConnectionFactory.h"
#include "RoomUtils.h"
#include "Utils.h"

namespace {

template <typename T>
inline auto weak(const std::shared_ptr<T>& strong) {
    return std::weak_ptr<T>(strong);
}

inline webrtc::MediaType fromString(const std::string& type) {
    if (type == webrtc::MediaTypeToString(webrtc::MediaType::AUDIO)) {
        return webrtc::MediaType::AUDIO;
    }
    if (type == webrtc::MediaTypeToString(webrtc::MediaType::VIDEO)) {
        return webrtc::MediaType::VIDEO;
    }
    if (type == webrtc::MediaTypeToString(webrtc::MediaType::DATA)) {
        return webrtc::MediaType::DATA;
    }
    return webrtc::MediaType::UNSUPPORTED;
}

rtc::scoped_refptr<webrtc::RtpSenderInterface>
    findSender(const webrtc::scoped_refptr<webrtc::PeerConnectionInterface>& pc,
               const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);

}

// https://github.com/livekit/client-sdk-js/blob/main/src/room/PCTransport.ts
namespace LiveKitCpp
{

Transport::Transport(SignalTarget target, TransportListener* listener,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::string& identity,
                     const std::string& prefferedAudioCodec,
                     const std::string& prefferedVideoCodec,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : RtcObject<TransportImpl, CreateSdpListener, SetSdpListener>(target, pcf, listener, conf, identity, logger)
    , _target(target)
    , _prefferedAudioCodec(prefferedAudioCodec)
    , _prefferedVideoCodec(prefferedVideoCodec)
{
    const auto impl = loadImpl();
    if (impl && impl->peerConnection()) {
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

bool Transport::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logInfo("request to set peer connection configuration");
            thread->PostTask([config, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    const auto pc = impl->peerConnection();
                    if (!pc) {
                        return;
                    }
                    auto res = pc->SetConfiguration(config);
                    if (res.ok()) {
                        impl->logVerbose("set of peer connection configuration successfully completed");
                        impl->notify(&TransportListener::onConfigurationSet, config);
                    }
                    else {
                        impl->logWebRTCError(res, "failed to set connection configuration configuration");
                        impl->notify(&TransportListener::onConfigurationSetFailure, std::move(res));
                    }
                }
            });
            return true;
        }
    }
    return false;
}

bool Transport::createDataChannel(const std::string& label, const webrtc::DataChannelInit& init)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logInfo("request to create '" + label + "' data channel");
            thread->PostTask([label, init, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    const auto pc = impl->peerConnection();
                    if (!pc) {
                        return;
                    }
                    auto result = pc->CreateDataChannelOrError(label, &init);
                    if (result.ok()) {
                        impl->logVerbose("data channel '" + label + "' has been created");
                        impl->notify(&TransportListener::onLocalDataChannelCreated,
                                     DataChannel::create(true, result.MoveValue()));
                    }
                    else {
                        impl->logWebRTCError(result.error(), "unable to create data channel '"
                                             + label + "'");
                        impl->notify(&TransportListener::onLocalDataChannelCreationFailure,
                                     label, init, result.MoveError());
                    }
                }
            });
            return true;
        }
    }
    return false;
}

bool Transport::addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                         const std::vector<std::string>& streamIds,
                         const std::vector<webrtc::RtpEncodingParameters>& initSendEncodings)
{
    if (track) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                const auto id = track->id();
                const auto kind = track->kind();
                impl->logInfo("request to adding local '" + id + "' " + kind + " track");
                thread->PostTask([id, kind, track = std::move(track), streamIds,
                                  initSendEncodings, implRef = weak(impl)]() {
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
                            impl->notify(&TransportListener::onLocalTrackAdded, result.MoveValue());
                        }
                        else {
                            impl->logWebRTCError(result.error(), "failed to add '" +
                                                 id + "' local " + kind + " track");
                            impl->notify(&TransportListener::onLocalTrackAddFailure,
                                         id, fromString(kind), streamIds, result.MoveError());
                        }
                    }
                });
                return true;
            }
        }
    }
    return false;
}

bool Transport::removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                const auto id = sender->id();
                const auto kind = webrtc::MediaTypeToString(sender->media_type());
                impl->logInfo("request to removal '" + id + "' local " + kind + " track");
                thread->PostTask([sender = std::move(sender), implRef = weak(impl)]() {
                    if (const auto impl = implRef.lock()) {
                        impl->removeTrackBySender(sender);
                    }
                });
                return true;
            }
        }
    }
    return false;
}

bool Transport::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                const auto id = track->id();
                const auto kind = track->kind();
                impl->logInfo("request to removal '" + id + "' local " + kind + " track");
                thread->PostTask([id, kind, track, implRef = weak(impl)]() {
                    if (const auto impl = implRef.lock()) {
                        if (const auto pc = impl->peerConnection()) {
                            impl->removeTrackBySender(findSender(pc, track));
                        }
                    }
                });
                return true;
            }
        }
    }
    return false;
}

void Transport::addIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    if (candidate) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                const auto info = candidate->candidate().ToSensitiveString();
                impl->logInfo("request to add (" + info + ") ICE candidate");
                thread->PostTask([info, candidate = std::move(candidate),
                                  implRef = weak(impl)]() mutable {
                    if (const auto impl = implRef.lock()) {
                        const auto pc = impl->peerConnection();
                        if (!pc) {
                            return;
                        }
                        auto handler = [info, implRef = std::move(implRef)](webrtc::RTCError error) {
                            if (const auto impl = implRef.lock()) {
                                if (error.ok()) {
                                    impl->logVerbose("ICE candidate (" + info +
                                                     ") has been added successfully");
                                    impl->notify(&TransportListener::onIceCandidateAdded);
                                }
                                else {
                                    impl->logWebRTCError(error, "failed to add ICE candidate (" +
                                                         info + ")");
                                    impl->notify(&TransportListener::onIceCandidateAddFailure,
                                                 std::move(error));
                                }
                            }
                        };
                        pc->AddIceCandidate(std::move(candidate), std::move(handler));
                    }
                });
            }
        }
    }
}

bool Transport::addTransceiver(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
                               const webrtc::RtpTransceiverInit& init)
{
    if (track) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                const auto id = track->id();
                const auto kind = track->kind();
                impl->logInfo("request to adding '" + id + "' " + kind + " transceiver");
                thread->PostTask([id, kind, track = std::move(track), init, implRef = weak(impl)]() {
                    if (const auto impl = implRef.lock()) {
                        const auto pc = impl->peerConnection();
                        if (!pc) {
                            return;
                        }
                        auto result = pc->AddTransceiver(std::move(track), init);
                        if (result.ok()) {
                            impl->logVerbose(kind + " transceiver '" + id + "' was added");
                            impl->notify(&TransportListener::onTransceiverAdded, result.MoveValue());
                        }
                        else {
                            impl->logWebRTCError(result.error(), "failed to add '" +
                                                 id + "' " + kind + " transceiver");
                            impl->notify(&TransportListener::onTransceiverAddFailure,
                                         id, fromString(kind), init, result.MoveError());
                        }
                    }
                });
                return true;
            }
        }
    }
    return false;
}

void Transport::queryReceiverStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback,
                                   const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver) const
{
    if (callback) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                thread->PostTask([receiver, callback, implRef = weak(impl)]() {
                    if (const auto impl = implRef.lock()) {
                        if (const auto pc = impl->peerConnection()) {
                            pc->GetStats(receiver, callback);
                        }
                    }
                });
            }
        }
    }
}

void Transport::querySenderStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback,
                                 const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (callback) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                thread->PostTask([sender, callback, implRef = weak(impl)]() {
                    if (const auto impl = implRef.lock()) {
                        if (const auto pc = impl->peerConnection()) {
                            pc->GetStats(sender, callback);
                        }
                    }
                });
            }
        }
    }
}

void Transport::createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kOffer));
            thread->PostTask([options, observer = _offerCreationObserver, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    if (const auto pc = impl->peerConnection()) {
                        pc->CreateOffer(observer.get(), options);
                    }
                }
            });
        }
    }
}

void Transport::createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logInfo("request to create " + sdpTypeToString(webrtc::SdpType::kAnswer));
            thread->PostTask([options, observer = _answerCreationObserver, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    if (const auto pc = impl->peerConnection()) {
                        pc->CreateAnswer(observer.get(), options);
                    }
                }
            });
        }
    }
}

void Transport::setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                impl->logInfo("request to set local " + desc->type());
                thread->PostTask([desc = patch(std::move(desc)),
                                  observer = _setLocalSdpObserver,
                                  implRef = weak(impl)]() mutable {
                    if (const auto impl = implRef.lock()) {
                        if (const auto pc = impl->peerConnection()) {
                            pc->SetLocalDescription(std::move(desc), observer);
                        }
                    }
                });
            }
        }
    }
}

void Transport::setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (const auto impl = loadImpl()) {
            if (const auto thread = impl->signalingThread()) {
                impl->logInfo("request to set remote " + desc->type());
                thread->PostTask([desc = std::move(desc),
                                  observer = _setRemoteSdpObserver,
                                  implRef = weak(impl)]() mutable {
                    if (const auto impl = implRef.lock()) {
                        if (const auto pc = impl->peerConnection()) {
                            pc->SetRemoteDescription(std::move(desc), observer);
                        }
                    }
                });
            }
        }
    }
}

const webrtc::SessionDescriptionInterface* Transport::localDescription() const
{
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            return pc->local_description();
        }
    }
    return nullptr;
}

const webrtc::SessionDescriptionInterface* Transport::remoteDescription() const
{
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            return pc->remote_description();
        }
    }
    return nullptr;
}

webrtc::PeerConnectionInterface::PeerConnectionState Transport::state() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->state();
    }
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

webrtc::PeerConnectionInterface::IceConnectionState Transport::iceConnectionState() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->iceConnectionState();
    }
    return webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed;
}

webrtc::PeerConnectionInterface::SignalingState Transport::signalingState() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->signalingState();
    }
    return webrtc::PeerConnectionInterface::SignalingState::kClosed;
}

webrtc::PeerConnectionInterface::IceGatheringState Transport::iceGatheringState() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->iceGatheringState();
    }
    return webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete;
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
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            return pc->GetTransceivers();
        }
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> Transport::receivers() const
{
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            return pc->GetReceivers();
        }
    }
    return {};
}

std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> Transport::senders() const
{
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            return pc->GetSenders();
        }
    }
    return {};
}

void Transport::setAudioPlayout(bool playout)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logVerbose(std::string(playout ? "enable" : "disable") +
                             " playout of received audio streams");
            thread->PostTask([playout, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    if (const auto pc = impl->peerConnection()) {
                        pc->SetAudioPlayout(playout);
                    }
                }
            });
        }
    }
}

void Transport::setAudioRecording(bool recording)
{
    if (const auto impl = loadImpl()) {
        if (const auto thread = impl->signalingThread()) {
            impl->logVerbose(std::string(recording ? "enable" : "disable") +
                             " recording of transmitted audio streams");
            thread->PostTask([recording, implRef = weak(impl)]() {
                if (const auto impl = implRef.lock()) {
                    if (const auto pc = impl->peerConnection()) {
                        pc->SetAudioRecording(recording);
                    }
                }
            });
        }
    }
}

bool Transport::valid() const
{
    if (const auto impl = loadImpl()) {
        return nullptr != impl->peerConnection();
    }
    return false;
}

void Transport::close()
{
    if (auto impl = dispose()) {
        _offerCreationObserver->setListener(nullptr);
        _answerCreationObserver->setListener(nullptr);
        _setLocalSdpObserver->setListener(nullptr);
        _setRemoteSdpObserver->setListener(nullptr);
        impl->close();
    }
}

std::unique_ptr<webrtc::SessionDescriptionInterface> Transport::patch(std::unique_ptr<webrtc::SessionDescriptionInterface> desc) const
{
    if (desc && (!_prefferedAudioCodec.empty() || !_prefferedVideoCodec.empty())) {
        SdpPatch patch(desc.get());
        patch.setCodec(_prefferedAudioCodec, webrtc::MediaType::AUDIO);
        patch.setCodec(_prefferedVideoCodec, webrtc::MediaType::VIDEO);
    }
    return desc;
}

void Transport::onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (const auto impl = loadImpl()) {
        if (desc) {
            impl->logVerbose(desc->type() + " created successfully");
        }
        impl->notify(&TransportListener::onSdpCreated, patch(std::move(desc)));
    }
}

void Transport::onFailure(webrtc::SdpType type, webrtc::RTCError error)
{
    if (const auto impl = loadImpl()) {
        impl->logWebRTCError(error, "failed to create " + sdpTypeToString(type));
        impl->notify(&TransportListener::onSdpCreationFailure, type, std::move(error));
    }
}

void Transport::onCompleted(bool local)
{
    if (const auto impl = loadImpl()) {
        if (const auto pc = impl->peerConnection()) {
            if (const auto desc = local ? pc->local_description() : pc->remote_description()) {
                impl->logVerbose(std::string(local ? "local " : "remote ") +
                                 desc->type() + " has been set successfully");
                impl->notify(&TransportListener::onSdpSet, local, desc);
            }
        }
    }
}

void Transport::onFailure(bool local, webrtc::RTCError error)
{
    if (const auto impl = loadImpl()) {
        impl->logWebRTCError(error, "failed to set " + std::string(local ? "local SDP" : "remote SDP"));
        impl->notify(&TransportListener::onSdpSetFailure, local, std::move(error));
    }
}

} // namespace LiveKitCpp


namespace {

rtc::scoped_refptr<webrtc::RtpSenderInterface>
    findSender(const webrtc::scoped_refptr<webrtc::PeerConnectionInterface>& pc,
               const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (pc && track) {
        for (const auto& sender : pc->GetSenders()) {
            if (sender->track() == track) {
                return sender;
            }
        }
    }
    return {};
}

}
