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
#ifdef WEBRTC_AVAILABLE
#include "RemoteParticipantsImpl.h"
#include "RemoteParticipantImpl.h"
#include <api/rtp_transceiver_interface.h>

namespace LiveKitCpp
{

RemoteParticipantsImpl::RemoteParticipantsImpl(TrackManager* trackManager,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    : DataChannelsStorage<>(logger)
    , _trackManager(trackManager)
{
}

RemoteParticipantsImpl::~RemoteParticipantsImpl()
{
    reset();
}

void RemoteParticipantsImpl::setInfo(const std::vector<ParticipantInfo>& infos)
{
    LOCK_WRITE_SAFE_OBJ(_participants);
    _participants->clear();
    if (!infos.empty()) {
        _participants->reserve(infos.size());
        LOCK_WRITE_SAFE_OBJ(_trackRefs);
        LOCK_WRITE_SAFE_OBJ(_orphans);
        _trackRefs->clear();
        for (const auto& info : infos) {
            auto participant = std::make_shared<RemoteParticipantImpl>(info);
            // fill tracks or keep references for the future orphans
            for (const auto& trackInfo : info._tracks) {
                const auto& sid = trackInfo._sid;
                const auto type = trackInfo._type;
                const auto orphan = _orphans->find(sid);
                if (orphan != _orphans->end()) {
                    addToParticipant(participant.get(), type, sid, orphan->second);
                    _orphans->erase(orphan);
                }
                else {
                    auto trackRef = std::make_pair(type, participant.get());
                    _trackRefs->insert(std::make_pair(sid, std::move(trackRef)));
                }
            }
            _participants->push_back(std::move(participant));
        }
    }
}

void RemoteParticipantsImpl::updateInfo(const std::vector<ParticipantInfo>& infos)
{
    if (!infos.empty()) {
        
    }
}

bool RemoteParticipantsImpl::addMedia(const rtc::scoped_refptr<webrtc::RtpTransceiverInterface>& transceiver)
{
    return transceiver && addMedia(transceiver->receiver());
}

bool RemoteParticipantsImpl::addMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    bool added = false;
    if (receiver) {
        if (const auto track = receiver->track()) {
            auto sid = receiver->id();
            if (!sid.empty()) {
                LOCK_WRITE_SAFE_OBJ(_trackRefs);
                LOCK_WRITE_SAFE_OBJ(_orphans);
                added = addToParticipant(sid, track);
                if (!added) {
                    added = addToOrphans(std::move(sid), track);
                }
            }
            else {
                logWarning("empty ID of track receiver");
            }
        }
    }
    return added;
}

bool RemoteParticipantsImpl::removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    if (receiver) {
        const auto sid = receiver->id();
        if (!sid.empty()) {
            LOCK_READ_SAFE_OBJ(_participants);
            {
                LOCK_WRITE_SAFE_OBJ(_orphans);
                _orphans->erase(sid);
            }
            {
                LOCK_WRITE_SAFE_OBJ(_trackRefs);
                _trackRefs->erase(sid);
            }
            for (const auto& participant : _participants.constRef()) {
                if (participant->removeMedia(sid)) {
                    break;
                }
            }
            return true;
        }
    }
    return false;
}

bool RemoteParticipantsImpl::addDataChannel(rtc::scoped_refptr<DataChannel> channel)
{
    return channel && !channel->local() && add(std::move(channel));
}

void RemoteParticipantsImpl::reset()
{
    clear();
    _orphans({});
    _trackRefs({});
    _participants({});
}

size_t RemoteParticipantsImpl::count() const
{
    LOCK_READ_SAFE_OBJ(_participants);
    return _participants->size();
}

std::shared_ptr<RemoteParticipant> RemoteParticipantsImpl::at(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_participants);
    if (index < _participants->size()) {
        return _participants->at(index);
    }
    return {};
}

std::shared_ptr<RemoteParticipant> RemoteParticipantsImpl::at(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_participants);
        for (const auto& participant : _participants.constRef()) {
            if (sid == participant->sid()) {
                return participant;
            }
        }
    }
    return {};
}

std::string_view RemoteParticipantsImpl::logCategory() const
{
    static const std::string_view category("remote_participants");
    return category;
}

bool RemoteParticipantsImpl::addToParticipant(const std::string& sid,
                                              const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track && !sid.empty()) {
        const auto it = _trackRefs->find(sid);
        if (it != _trackRefs->end()) {
            addToParticipant(it->second.second, it->second.first, sid, track);
            _trackRefs->erase(it);
            return true;
        }
    }
    return false;
}

void RemoteParticipantsImpl::addToParticipant(RemoteParticipantImpl* participant,
                                              TrackType type, const std::string& sid,
                                              const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) const
{
    if (participant && track) {
        bool added = false;
        switch (type) {
            case TrackType::Audio:
                added = participant->addAudio(sid, _trackManager, track);
                break;
            case TrackType::Video:
                added = participant->addVideo(sid, _trackManager, track);
                break;
            default:
                break;
        }
        if (!added) {
            // TODO: log error
        }
    }
}

bool RemoteParticipantsImpl::addToOrphans(std::string sid,
                                          const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track && !sid.empty()) {
        _orphans->insert(std::make_pair(std::move(sid), track));
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
#endif
