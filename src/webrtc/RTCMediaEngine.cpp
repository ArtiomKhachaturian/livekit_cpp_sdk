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
#include "PeerConnectionFactory.h"
#include "rtc/AddTrackRequest.h"
#include "rtc/MuteTrackRequest.h"
#include "rtc/TrackPublishedResponse.h"

namespace {

inline std::string dcType(bool local) {
    return local ? "local" : "remote";
}

}

namespace LiveKitCpp
{

RTCMediaEngine::RTCMediaEngine(PeerConnectionFactory* pcf,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _pcf(pcf)
    , _microphone(this, true)
{
}

RTCMediaEngine::~RTCMediaEngine()
{
    RTCMediaEngine::cleanup(false);
}

void RTCMediaEngine::cleanup(bool /*error*/)
{
    _microphone.resetTrackSender();
    _localDCs({});
    _remoteDCs({});
}

LocalTrack* RTCMediaEngine::localTrack(const std::string& id, cricket::MediaType type)
{
    if (!id.empty()) {
        if (cricket::MEDIA_TYPE_UNSUPPORTED == type) {
            if (id == _microphone.cid()) {
                return &_microphone;
            }
        }
        else {
            switch (type) {
                case cricket::MEDIA_TYPE_AUDIO:
                    if (id == _microphone.cid()) {
                        return &_microphone;
                    }
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
                    break;
                default:
                    break;
            }
        }
    }
    return nullptr;
}

void RTCMediaEngine::sendAddTrack(const LocalTrack* track)
{
    if (track && !closed()) {
        AddTrackRequest request;
        if (track->fillRequest(request)) {
            if (SendResult::TransportError == sendAddTrack(request)) {
                // TODO: log error
            }
        }
        else {
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

void RTCMediaEngine::onTrackPublished(uint64_t, const TrackPublishedResponse& published)
{
    if (const auto track = localTrack(published._cid)) {
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

void RTCMediaEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (sender) {
        if (const auto track = localTrack(sender->id(), sender->media_type())) {
            const auto result = track->setTrackSender(sender);
            if (SetSenderResult::Accepted == result) {
                sendAddTrack(track);
            }
            else if (SetSenderResult::Rejected == result) {
                if (!remove(std::move(sender))) {
                    // TODO: maybe log as warn
                }
            }
        }
    }
}

void RTCMediaEngine::onLocalTrackAddFailure(const std::string& id,
                                            cricket::MediaType type,
                                            const std::vector<std::string>&,
                                            webrtc::RTCError)
{
    if (const auto track = localTrack(id, type)) {
        track->resetTrackSender();
    }
}

void RTCMediaEngine::onLocalTrackRemoved(const std::string& id, cricket::MediaType type)
{
    if (const auto track = localTrack(id, type)) {
        track->resetTrackSender();
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

webrtc::scoped_refptr<webrtc::AudioTrackInterface> RTCMediaEngine::
    createAudio(const std::string& label, const cricket::AudioOptions& options)
{
    if (_pcf) {
        if (const auto source = _pcf->CreateAudioSource(options)) {
            return _pcf->CreateAudioTrack(label, source.get());
        }
    }
    return {};
}

void RTCMediaEngine::notifyAboutMuteChanges(const std::string& trackSid, bool muted)
{
    if (!trackSid.empty()) {
        if (SendResult::TransportError == sendMuteTrack({._sid = trackSid, ._muted = muted})) {
            // TODO: log error
        }
    }
}

void RTCMediaEngine::onStateChange(DataChannel* channel)
{
    if (channel && canLogInfo()) {
        logVerbose(dcType(channel->local()) + " data channel '" +
                   channel->label() + "' state have changed");
    }
}

void RTCMediaEngine::onMessage(DataChannel* channel,
                               const webrtc::DataBuffer& /*buffer*/)
{
    if (channel && canLogInfo()) {
        logVerbose("a message buffer was successfully received for '" +
                   channel->label() + "' " + dcType(channel->local()) +
                   " data channel");
    }
}

void RTCMediaEngine::onBufferedAmountChange(DataChannel* /*channelType*/,
                                            uint64_t /*sentDataSize*/)
{
}

} // namespace LiveKitCpp
