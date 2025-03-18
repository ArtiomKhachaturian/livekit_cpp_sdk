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
#include "RemoteParticipants.h"
#include "RemoteParticipantsListener.h"
#include "RemoteParticipantImpl.h"
#include "Seq.h"
#include "rtc/ParticipantInfo.h"
#include <api/rtp_transceiver_interface.h>

namespace {

using namespace LiveKitCpp;

inline bool compareParticipantInfo(const ParticipantInfo& l, const ParticipantInfo& r) {
    return l._sid == r._sid;
}

}

namespace LiveKitCpp
{
    
RemoteParticipants::RemoteParticipants(TrackManager* trackManager,
                                       RemoteParticipantsListener* listener,
                                       const std::shared_ptr<Bricks::Logger>& logger)
    : DataChannelsStorage<>(logger)
    , _trackManager(trackManager)
    , _listener(listener)
{
}

void RemoteParticipants::setInfo(const std::vector<ParticipantInfo>& infos)
{
    LOCK_WRITE_SAFE_OBJ(_participants);
    clearParticipants();
    if (!infos.empty()) {
        _participants->reserve(infos.size());
        for (const auto& info : infos) {
            if (ParticipantState::Disconnected != info._state) {
                addParticipant(std::make_shared<RemoteParticipantImpl>(info));
            }
        }
    }
}

void RemoteParticipants::updateInfo(const std::vector<ParticipantInfo>& infos)
{
    if (!infos.empty()) {
        using SeqType = Seq<ParticipantInfo>;
        LOCK_WRITE_SAFE_OBJ(_participants);
        const auto current = this->infos();
        const auto added = SeqType::difference(infos, current, compareParticipantInfo);
        const auto removed = SeqType::difference(current, infos, compareParticipantInfo);
        const auto updated = SeqType::intersection(current, infos, compareParticipantInfo);
        // add new
        for (const auto& info : added) {
            addParticipant(std::make_shared<RemoteParticipantImpl>(info));
        }
        // remove
        for (const auto& info : removed) {
            removeParticipant(info._sid);
        }
        // update existed
        for (const auto& info : updated) {
            if (const auto ndx = findBySid(info._sid)) {
                const auto participant = _participants->at(ndx.value());
                participant->setInfo(info);
                if (ParticipantState::Disconnected == info._state) {
                    _participants->erase(_participants->begin() + ndx.value());
                    removeParticipant(participant);
                }
            }
        }
    }
}

bool RemoteParticipants::addMedia(const rtc::scoped_refptr<webrtc::RtpTransceiverInterface>& transceiver)
{
    return transceiver && addMedia(transceiver->receiver());
}

bool RemoteParticipants::addMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    bool added = false;
    if (receiver) {
        if (const auto track = receiver->track()) {
            auto sid = receiver->id();
            if (!sid.empty()) {
                LOCK_WRITE_SAFE_OBJ(_orphans);
                added = addMedia(sid, track);
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

bool RemoteParticipants::removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    if (receiver) {
        const auto sid = receiver->id();
        if (!sid.empty()) {
            LOCK_READ_SAFE_OBJ(_participants);
            {
                LOCK_WRITE_SAFE_OBJ(_orphans);
                _orphans->erase(sid);
            }
            const auto type = receiver->media_type();
            for (const auto& participant : _participants.constRef()) {
                if (cricket::MEDIA_TYPE_VIDEO == type) {
                    if (participant->removeVideo(sid)) {
                        break;
                    }
                }
                else if (cricket::MEDIA_TYPE_AUDIO == type) {
                    if (participant->removeAudio(sid)) {
                        break;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool RemoteParticipants::addDataChannel(rtc::scoped_refptr<DataChannel> channel)
{
    return channel && !channel->local() && add(std::move(channel));
}

void RemoteParticipants::reset()
{
    clear();
    _orphans({});
    LOCK_WRITE_SAFE_OBJ(_participants);
    clearParticipants();
}

size_t RemoteParticipants::count() const
{
    LOCK_READ_SAFE_OBJ(_participants);
    return _participants->size();
}

std::shared_ptr<RemoteParticipant> RemoteParticipants::at(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_participants);
    if (index < _participants->size()) {
        return _participants->at(index);
    }
    return {};
}

std::shared_ptr<RemoteParticipant> RemoteParticipants::at(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_participants);
        if (const auto ndx = findBySid(sid)) {
            return _participants->at(ndx.value());
        }
    }
    return {};
}

std::string_view RemoteParticipants::logCategory() const
{
    static const std::string_view category("remote_participants");
    return category;
}

std::vector<ParticipantInfo> RemoteParticipants::infos() const
{
    if (const auto s = _participants->size()) {
        std::vector<ParticipantInfo> output;
        output.reserve(s);
        for (const auto& participant : _participants.constRef()) {
            output.push_back(participant->info());
        }
        return output;
    }
    return {};
}

bool RemoteParticipants::addMedia(const std::string& sid,
                                  const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track && !sid.empty()) {
        std::shared_ptr<RemoteParticipantImpl> target;
        TrackType trackType;
        {
            LOCK_READ_SAFE_OBJ(_participants);
            for (const auto& participant : _participants.constRef()) {
                if (const auto type = participant->trackType(sid)) {
                    target = participant;
                    trackType = type.value();
                    break;
                }
            }
        }
        if (target) {
            addMedia(target, trackType, sid, track);
            return true;
        }
    }
    return false;
}

void RemoteParticipants::addMedia(const std::shared_ptr<RemoteParticipantImpl>& participant,
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

bool RemoteParticipants::addToOrphans(std::string sid,
                                      const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track && !sid.empty()) {
        _orphans->insert(std::make_pair(std::move(sid), track));
        return true;
    }
    return false;
}

void RemoteParticipants::addParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant)
{
    if (participant) {
        {
            LOCK_WRITE_SAFE_OBJ(_orphans);
            // fill tracks or keep references for the future orphans
            for (const auto& trackInfo : participant->info()._tracks) {
                const auto& sid = trackInfo._sid;
                const auto type = trackInfo._type;
                const auto orphan = _orphans->find(sid);
                if (orphan != _orphans->end()) {
                    addMedia(participant, type, sid, orphan->second);
                    _orphans->erase(orphan);
                }
            }
        }
        _participants->push_back(participant);
        if (_listener) {
            _listener->onParticipantAdded(participant->sid());
        }
    }
}

void RemoteParticipants::removeParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant)
{
    if (participant) {
        {
            LOCK_WRITE_SAFE_OBJ(_orphans);
            for (const auto& trackInfo : participant->info()._tracks) {
                _orphans->erase(trackInfo._sid);
            }
        }
        participant->reset();
        if (_listener) {
            _listener->onParticipantRemoved(participant->sid());
        }
    }
}

void RemoteParticipants::removeParticipant(const std::string& sid)
{
    if (!sid.empty()) {
        std::shared_ptr<RemoteParticipantImpl> participant;
        if (const auto ndx = findBySid(sid)) {
            participant = _participants->at(ndx.value());
            _participants->erase(_participants->begin() + ndx.value());
        }
        if (participant) {
            removeParticipant(participant);
        }
    }
}

void RemoteParticipants::clearParticipants()
{
    for (const auto& participant : _participants.take()) {
        removeParticipant(participant);
    }
}

std::optional<size_t> RemoteParticipants::findBySid(const std::string& sid) const
{
    if (!sid.empty()) {
        if (const auto s = _participants->size()) {
            for (size_t i = 0U; i < s; ++i) {
                if (sid == _participants->at(i)->sid()) {
                    return i;
                }
            }
        }
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
#endif
