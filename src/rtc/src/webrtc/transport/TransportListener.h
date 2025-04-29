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
#pragma once // TransportListener.h
#include "DataChannel.h"
#include <api/jsep.h>
#include <api/peer_connection_interface.h>
#include <memory>

namespace LiveKitCpp
{

class Transport;
enum class SignalTarget;

/**
 * @class WebRtcObserver
 * @brief Extends webrtc::PeerConnectionObserver interface with additional functionality.
 *
 * This class provides various callbacks for handling WebRTC events, such as SDP creation, ICE candidate management,
 * data channel operations, and track management.
 */
class TransportListener
{
public:
    /**
     * @brief Callback triggered after createOffer and createAnswer, transferring the ownership of the `desc`.
     *
     * @param target The target of the signal.
     * @param desc The session description interface.
     */
    virtual void onSdpCreated(SignalTarget /*target*/,
                              std::unique_ptr<webrtc::SessionDescriptionInterface> /*desc*/) {}

    /**
     * @brief Callback triggered when SDP creation fails.
     *
     * The callback takes an RTCError, which consists of an error code and a string.
     * RTCError is non-copyable and must be passed using std::move.
     *
     * @param target The target of the signal.
     * @param type The type of SDP.
     * @param error The error that occurred.
     */
    virtual void onSdpCreationFailure(SignalTarget /*target*/,
                                      webrtc::SdpType /*type*/,
                                      webrtc::RTCError /*error*/) {}

    /**
     * @brief Callback invoked as soon as setLocalDescription/setRemoteDescription operations complete.
     *
     * Allows the observer to examine the effects of the operation without delay.
     *
     * @param target The target of the signal.
     * @param local Indicates whether the description is local.
     * @param desc The session description interface.
     */
    virtual void onSdpSet(SignalTarget /*target*/, bool /*local*/,
                          const webrtc::SessionDescriptionInterface* /*desc*/) {}

    /**
     * @brief Callback triggered when something goes wrong during setLocalDescription/setRemoteDescription operations.
     *
     * @param target The target of the signal.
     * @param local Indicates whether the description is local.
     * @param error The error that occurred.
     */
    virtual void onSdpSetFailure(SignalTarget /*target*/, bool /*local*/,
                                 webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when the RTC configuration is successfully set.
     *
     * @param target The target of the signal.
     * @param config The RTC configuration that was set.
     */
    virtual void onConfigurationSet(SignalTarget /*target*/,
                                    const webrtc::PeerConnectionInterface::RTCConfiguration& /*config*/) {}

    /**
     * @brief Called when setting the RTC configuration fails.
     *
     * @param target The target of the signal.
     * @param error The error that occurred while setting the configuration.
     */
    virtual void onConfigurationSetFailure(SignalTarget /*target*/,
                                           webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when an ICE candidate is successfully added.
     *
     * @param target The target of the signal.
     */
    virtual void onIceCandidateAdded(SignalTarget /*target*/) {}

    /**
     * @brief Called when adding an ICE candidate fails.
     *
     * @param target The target of the signal.
     * @param error The error that occurred while adding the ICE candidate.
     */
    virtual void onIceCandidateAddFailure(SignalTarget /*target*/,
                                          webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when a local data channel is successfully created.
     *
     * @param target The target of the signal.
     * @param channel The data channel that was created.
     */
    virtual void onLocalDataChannelCreated(SignalTarget /*target*/,
                                           rtc::scoped_refptr<DataChannel> /*channel*/) {}

    /**
     * @brief Called when creating a local data channel fails.
     *
     * @param target The target of the signal.
     * @param label The label of the data channel.
     * @param init The initialization parameters of the data channel.
     * @param error The error that occurred while creating the data channel.
     */
    virtual void onLocalDataChannelCreationFailure(SignalTarget /*target*/,
                                                   const std::string& /*label*/,
                                                   const webrtc::DataChannelInit& /*init*/,
                                                   webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when a local track is successfully added.
     *
     * @param target The target of the signal.
     * @param sender The sender of the track that was added.
     */
    virtual void onLocalTrackAdded(SignalTarget /*target*/,
                                   rtc::scoped_refptr<webrtc::RtpSenderInterface> /*sender*/) {}

    /**
     * @brief Called when adding a local track fails.
     *
     * @param target The target of the signal.
     * @param id The ID of the track.
     * @param type The kind of the track (e.g., audio, video).
     * @param streamIds The IDs of the streams to which the track belongs.
     * @param error The error that occurred while adding the track.
     */
    virtual void onLocalTrackAddFailure(SignalTarget /*target*/,
                                        const std::string& /*id*/,
                                        cricket::MediaType /*type*/,
                                        const std::vector<std::string>& /*streamIds*/,
                                        webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when a local track is successfully removed.
     *
     * @param target The target of the signal.
     * @param id The ID of the track.
     * @param type The kind of the track (e.g., audio, video).
     * @param streamIds The IDs of the streams to which the track belongs.
     */
    virtual void onLocalTrackRemoved(SignalTarget /*target*/,
                                     const std::string& /*id*/,
                                     cricket::MediaType /*type*/,
                                     const std::vector<std::string>& /*streamIds*/) {}

    /**
     * @brief Called when removing a local track fails.
     *
     * @param target The target of the signal.
     * @param id The ID of the track.
     * @param type The kind of the track (e.g., audio, video).
     * @param streamIds The IDs of the streams from which the track was to be removed.
     * @param error The error that occurred while removing the track.
     */
    virtual void onLocalTrackRemoveFailure(SignalTarget /*target*/,
                                           const std::string& /*id*/,
                                           cricket::MediaType /*type*/,
                                           const std::vector<std::string>& /*streamIds*/,
                                           webrtc::RTCError /*error*/) {}

    /**
     * @brief Called when an RTP transceiver is successfully added.
     *
     * @param target The target of the signal.
     * @param transceiver The RTP transceiver that was added.
     */
    virtual void onTransceiverAdded(SignalTarget /*target*/,
                                    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> /*transceiver*/) {}

    /**
     * @brief Called when adding an RTP transceiver fails.
     *
     * @param target The target of the signal.
     * @param id The ID of the transceiver.
     * @param type The kind of the transceiver (e.g., audio, video).
     * @param init The initialization parameters of the transceiver.
     * @param error The error that occurred while adding the transceiver.
     */
    virtual void onTransceiverAddFailure(SignalTarget /*target*/,
                                         const std::string& /*id*/,
                                         cricket::MediaType /*type*/,
                                         const webrtc::RtpTransceiverInit& /*init*/,
                                         webrtc::RTCError /*error*/) {}

    /**
     * @brief Triggered when the SignalingState changes.
     *
     * @param target The target of the signal.
     * @param newState The new signaling state.
     */
    virtual void onSignalingChange(SignalTarget /*target*/,
                                   webrtc::PeerConnectionInterface::SignalingState /*newState*/) {}

    /**
     * @brief Used to fire spec-compliant onnegotiationneeded events.
     *
     * The observer is responsible for queuing a task to maybe fire the event. The event identified using `event_id`
     * must only fire if PeerConnection::ShouldFireNegotiationNeededEvent() returns true since it is possible for the event
     * to become invalidated by subsequently chained operations.
     *
     * @param target The target of the signal.
     * @param eventId The event ID.
     */
    virtual void onNegotiationNeededEvent(SignalTarget /*target*/, uint32_t /*eventId*/) {}

    /**
     * @brief Triggered when media is received on a new stream from a remote peer.
     *
     * @param target The target of the signal.
     * @param stream The media stream that was added.
     */
    virtual void onRemoteStreamAdded(SignalTarget /*target*/,
                                     rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}

    /**
     * @brief Triggered when a remote peer closes a stream.
     *
     * @param target The target of the signal.
     * @param stream The media stream that was removed.
     */
    virtual void onRemoteStreamRemoved(SignalTarget /*target*/,
                                       rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) {}
    /**
     * @brief Triggered when a remote peer opens a data channel.
     *
     * @param target The signal target.
     * @param channel The data channel interface.
     */
    virtual void onRemoteDataChannelOpened(SignalTarget /*target*/,
                                           rtc::scoped_refptr<DataChannel> /*channel*/) {}

    /**
     * @brief Called any time the standards-compliant IceConnectionState changes.
     *
     * @param target The signal target.
     * @param newState The new ICE connection state.
     */
    virtual void onIceConnectionChange(SignalTarget /*target*/,
                                       webrtc::PeerConnectionInterface::IceConnectionState /*newState*/) {}

    /**
     * @brief Called any time the PeerConnectionState changes.
     *
     * @param target The signal target.
     * @param newState The new PeerConnection state.
     */
    virtual void onConnectionChange(SignalTarget /*target*/,
                                    webrtc::PeerConnectionInterface::PeerConnectionState /*newState*/) {}

    /**
     * @brief Called any time the IceGatheringState changes.
     *
     * @param target The signal target.
     * @param newState The new ICE gathering state.
     */
    virtual void onIceGatheringChange(SignalTarget /*target*/,
                                      webrtc::PeerConnectionInterface::IceGatheringState /*newState*/) {}

    /**
     * @brief A new ICE candidate has been gathered.
     *
     * @param target The signal target.
     * @param candidate The ICE candidate interface.
     */
    virtual void onIceCandidateGathered(SignalTarget /*target*/,
                                        const webrtc::IceCandidateInterface* /*candidate*/) {}

    /**
     * @brief Gathering of an ICE candidate failed.
     *
     * @see https://w3c.github.io/webrtc-pc/#event-icecandidateerror
     *
     * @param target The signal target.
     * @param address The address.
     * @param port The port.
     * @param url The URL.
     * @param errorCode The error code.
     * @param errorText The error text.
     */
    virtual void onIceCandidateGatheringError(SignalTarget /*target*/,
                                              const std::string& /*address*/, int /*port*/,
                                              const std::string& /*url*/,
                                              int /*errorCode*/, const std::string& /*errorText*/) {}

    /**
     * @brief Ice candidates have been removed.
     *
     * @param target The signal target.
     * @param candidates The removed ICE candidates.
     */
    virtual void onIceCandidatesRemoved(SignalTarget /*target*/,
                                        const std::vector<cricket::Candidate>& /*candidates*/) {}

    /**
     * @brief Called when the ICE connection receiving status changes.
     *
     * @param target The signal target.
     * @param receiving The new receiving status.
     */
    virtual void onIceConnectionReceivingChange(SignalTarget /*target*/,
                                                bool /*receiving*/) {}

    /**
     * @brief Called when the selected candidate pair for the ICE connection changes.
     *
     * @param target The signal target.
     * @param event The candidate pair change event.
     */
    virtual void onIceSelectedCandidatePairChanged(SignalTarget /*target*/,
                                                   const cricket::CandidatePairChangeEvent& /*event*/) {}

    /**
     * @brief Called when a receiver and its track are created.
     * @note This is called with both Plan B and Unified Plan semantics. Unified
     * Plan users should prefer onRemoteTrackAdded. This method is only called for backwards compatibility.
     *
     * @param target The signal target.
     * @param receiver The RTP receiver interface.
     * @param streams The associated media streams.
     */
    virtual void onRemoteTrackAdded(SignalTarget /*target*/,
                                    rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/,
                                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& /*streams*/) {}

    /**
     * @brief Called when signaling indicates that media will no longer be received on a track.
     * @note With Plan B semantics, the given receiver will have been removed from the
     * PeerConnection and the track muted. With Unified Plan semantics, the receiver
     * will remain but the transceiver will have changed direction to either sendonly or inactive.
     * @see https://w3c.github.io/webrtc-pc/#process-remote-track-removal
     *
     * @param target The signal target.
     * @param receiver The RTP receiver interface.
     */
    virtual void onRemotedTrackRemoved(SignalTarget /*target*/,
                                       rtc::scoped_refptr<webrtc::RtpReceiverInterface> /*receiver*/) {}

    /**
     * @brief Called when an interesting usage is detected by WebRTC.
     * @note The heuristics for defining what constitutes "interesting" are implementation-defined.
     *
     * @param target The signal target.
     * @param usagePattern The usage pattern detected.
     */
    virtual void onInterestingUsage(SignalTarget /*target*/,
                                    int /*usagePattern*/) {}
protected:
    virtual ~TransportListener() = default;
};

} // namespace LiveKitCpp
