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
#include "RTCMediaEngine.h"
#include "DataChannel.h"
#include "RemoteTrack.h"
#include "rtc/AddTrackRequest.h"
#include "rtc/MuteTrackRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"

namespace {

inline std::string dcType(bool local) {
    return local ? "local" : "remote";
}

}

namespace LiveKitCpp
{

RTCMediaEngine::RTCMediaEngine(const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _microphone(this, true, logger)
{
}

RTCMediaEngine::~RTCMediaEngine()
{
    cleanupLocalResources();
    cleanupRemoteResources();
}

void RTCMediaEngine::addLocalResourcesToTransport()
{
    _microphone.addToTransport();
}

void RTCMediaEngine::cleanupLocalResources()
{
    _microphone.resetMedia();
    _pendingLocalMedias({});
}

void RTCMediaEngine::cleanupRemoteResources()
{
    _localDCs({});
    _remoteDCs({});
    _remoteTracks({});
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
    RTCMediaEngine::pendingLocalMedia()
{
    LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> medias;
    medias.reserve(_pendingLocalMedias->size());
    for (auto it = _pendingLocalMedias->begin(); it != _pendingLocalMedias->end(); ++it) {
        medias.push_back(std::move(it->second));
    }
    _pendingLocalMedias->clear();
    return medias;
}

bool RTCMediaEngine::addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track) {
        auto id = track->id();
        if (!id.empty()) {
            LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
            _pendingLocalMedias->insert(std::make_pair(std::move(id), track));
            return true;
        }
    }
    return false;
}

bool RTCMediaEngine::removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track) {
        const auto id = track->id();
        if (!id.empty()) {
            LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
            return _pendingLocalMedias->erase(track->id()) > 0U;
        }
    }
    return false;
}

LocalTrack* RTCMediaEngine::localTrack(const std::string& id, bool cid)
{
    if (!id.empty()) {
        if (id == (cid ? _microphone.cid() : _microphone.sid())) {
            return &_microphone;
        }
    }
    return nullptr;
}

LocalTrack* RTCMediaEngine::localTrack(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    if (sender) {
        return localTrack(sender->id(), true);
    }
    return nullptr;
}

void RTCMediaEngine::sendAddTrack(const LocalTrack* track)
{
    if (track && track->live() && !closed()) {
        AddTrackRequest request;
        track->fillRequest(&request);
        if (SendResult::TransportError == sendAddTrack(request)) {
            // TODO: log error
        }
    }
}

void RTCMediaEngine::addDataChannelToList(rtc::scoped_refptr<DataChannel> channel,
                                          DataChannels& list)
{
    if (channel) {
        const auto label = channel->label();
        const auto local = channel->local();
        if (label.empty()) {
            logWarning("unnamed " + dcType(local) + " data channel, processing denied");
        }
        else {
            auto it = list.find(label);
            if (it == list.end()) {
                list[label] = channel;
                logVerbose(dcType(local) + " data channel '" + channel->label() +
                           "' was added to list for observation");
            }
            else {
                logWarning(dcType(local) + " data channel '" + channel->label() +
                           "' is already present but will be overwritten");
                it->second = channel;
            }
            channel->setListener(this);
        }
    }
}

void RTCMediaEngine::onTrackPublished(const TrackPublishedResponse& published)
{
    if (const auto track = localTrack(published._cid, true)) {
        track->setSid(published._track._sid);
        // reconcile track mute status.
        // if server's track mute status doesn't match actual, we'll have to update
        // the server's copy
        const auto muted = track->muted();
        if (muted != published._track._muted) {
            notifyAboutMuteChanges(published._track._sid, muted);
        }
    }
}

void RTCMediaEngine::onTrackUnpublished(const TrackUnpublishedResponse& unpublished)
{
    if (const auto track = localTrack(unpublished._trackSid, false)) {
        track->resetMedia();
    }
}

void RTCMediaEngine::onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                                   webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                                   webrtc::PeerConnectionInterface::PeerConnectionState subscriberState)
{
    switch (publisherState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
            // publish local tracks
            sendAddTrack(&_microphone);
            break;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            cleanupLocalResources();
            break;
        default:
            break;
    }
    switch (subscriberState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            cleanupRemoteResources();
            break;
        default:
            break;
    }
 }

void RTCMediaEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (const auto track = localTrack(sender)) {
        track->notifyThatMediaAddedToTransport();
        sendAddTrack(track);
    }
}

void RTCMediaEngine::onLocalTrackAddFailure(const std::string& id,
                                            cricket::MediaType,
                                            const std::vector<std::string>&,
                                            webrtc::RTCError)
{
    if (const auto track = localTrack(id, true)) {
        track->resetMedia();
    }
}

void RTCMediaEngine::onLocalTrackRemoved(const std::string& id, cricket::MediaType)
{
    if (const auto track = localTrack(id, true)) {
        track->resetMedia(false);
    }
}

void RTCMediaEngine::onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (transceiver) {
        TrackManager* manager = this;
        auto track = std::make_unique<RemoteTrack>(manager, transceiver->receiver());
        auto sid = track->sid();
        LOCK_WRITE_SAFE_OBJ(_remoteTracks);
        _remoteTracks->insert(std::make_pair(std::move(sid), std::move(track)));
    }
}

void RTCMediaEngine::onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (receiver) {
        const auto sid = receiver->id();
        LOCK_WRITE_SAFE_OBJ(_remoteTracks);
        _remoteTracks->erase(sid);
    }
}

void RTCMediaEngine::onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        LOCK_WRITE_SAFE_OBJ(_localDCs);
        addDataChannelToList(std::move(channel), _localDCs.ref());
    }
}

void RTCMediaEngine::onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        LOCK_WRITE_SAFE_OBJ(_remoteDCs);
        addDataChannelToList(std::move(channel), _remoteDCs.ref());
    }
}

void RTCMediaEngine::notifyAboutMuteChanges(const std::string& trackSid, bool muted)
{
    if (!trackSid.empty()) {
        const auto result = sendMuteTrack({._sid = trackSid, ._muted = muted});
        if (SendResult::TransportError == result) {
            // TODO: log error
        }
    }
}

void RTCMediaEngine::onStateChange(DataChannel* channel)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose(dcType(channel->local()) + " data channel '" +
                       channel->label() + "' state has been changed to " +
                       dataStateToString(channel->state()));
        }
        
    }
}

void RTCMediaEngine::onMessage(DataChannel* channel,
                               const webrtc::DataBuffer& /*buffer*/)
{
    if (channel) {
        if (canLogVerbose()) {
            logVerbose("a message buffer was successfully received for '" +
                       channel->label() + "' " + dcType(channel->local()) +
                       " data channel");
        }
    }
}

void RTCMediaEngine::onBufferedAmountChange(DataChannel* /*channelType*/,
                                            uint64_t /*sentDataSize*/)
{
}

void RTCMediaEngine::onSendError(DataChannel* channel, webrtc::RTCError error)
{
    
}

} // namespace LiveKitCpp
