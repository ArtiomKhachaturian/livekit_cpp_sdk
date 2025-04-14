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
#include "RemoteParticipants.h"
#include "RemoteParticipantImpl.h"
#include "DataChannel.h"
#include "Seq.h"
#include "RemoteParticipantsListener.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"
#include <api/rtp_transceiver_interface.h>

namespace {

using namespace LiveKitCpp;

inline bool compareParticipantInfo(const ParticipantInfo& l, const ParticipantInfo& r) {
    return l._sid == r._sid;
}

}

namespace LiveKitCpp
{
    
RemoteParticipants::RemoteParticipants(E2ESecurityFactory* securityFactory,
                                       RemoteParticipantsListener* listener,
                                       const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _securityFactory(securityFactory)
    , _listener(listener)
    , _nonBindedReceivers(std::make_shared<NonBindedRtpReceivers>())
{
}

RemoteParticipants::~RemoteParticipants()
{
    reset();
}

bool RemoteParticipants::setRemoteSideTrackMute(const std::string& sid, bool mute)
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_participants);
        for (const auto& participant : _participants.constRef()) {
            if (participant->setRemoteSideTrackMute(sid, mute)) {
                return true;
            }
        }
    }
    return false;
}

void RemoteParticipants::setInfo(const std::vector<ParticipantInfo>& infos)
{
    LOCK_WRITE_SAFE_OBJ(_participants);
    clearParticipants();
    if (!infos.empty()) {
        _participants->reserve(infos.size());
        for (const auto& info : infos) {
            if (ParticipantState::Disconnected != info._state) {
                addParticipant(std::make_shared<RemoteParticipantImpl>(_securityFactory,
                                                                       _nonBindedReceivers,
                                                                       info));
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
            addParticipant(std::make_shared<RemoteParticipantImpl>(_securityFactory,
                                                                   _nonBindedReceivers,
                                                                   info));
        }
        // remove
        for (const auto& info : removed) {
            removeParticipant(info._sid);
        }
        // update existed
        for (const auto& info : updated) {
            if (const auto ndx = participantIndexBySid(info._sid)) {
                const auto participant = _participants->at(ndx.value());
                participant->setInfo(info);
                if (ParticipantState::Disconnected == info._state) {
                    _participants->erase(_participants->begin() + ndx.value());
                    dispose(participant);
                }
            }
        }
    }
}

bool RemoteParticipants::addMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    if (receiver) {
        const auto type = receiver->media_type();
        LOCK_READ_SAFE_OBJ(_participants);
        for (const auto& participant : _participants.constRef()) {
            switch (type) {
                case cricket::MEDIA_TYPE_AUDIO:
                    if (participant->addAudio(receiver)) {
                        return true;
                    }
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
                    if (participant->addVideo(receiver)) {
                        return true;
                    }
                    break;
                default:
                    break;
            }
        }
        if (_nonBindedReceivers->add(receiver)) {
            return true;
        }
    }
    return false;
}

bool RemoteParticipants::removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    if (receiver) {
        const auto sid = receiver->id();
        if (!sid.empty()) {
            _nonBindedReceivers->take(sid);
            LOCK_READ_SAFE_OBJ(_participants);
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

void RemoteParticipants::reset()
{
    _nonBindedReceivers->clear();
    LOCK_WRITE_SAFE_OBJ(_participants);
    clearParticipants();
}

size_t RemoteParticipants::count() const
{
    LOCK_READ_SAFE_OBJ(_participants);
    return _participants->size();
}

std::shared_ptr<RemoteParticipantImpl> RemoteParticipants::at(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_participants);
    if (index < _participants->size()) {
        return _participants->at(index);
    }
    return {};
}

std::shared_ptr<RemoteParticipantImpl> RemoteParticipants::at(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_participants);
        if (const auto ndx = participantIndexBySid(sid)) {
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

void RemoteParticipants::addParticipant(const std::shared_ptr<RemoteParticipantImpl>& participant)
{
    if (participant) {
        _participants->push_back(participant);
        if (_listener) {
            _listener->onParticipantAdded(participant->sid());
        }
    }
}

void RemoteParticipants::removeParticipant(const std::string& sid)
{
    if (!sid.empty()) {
        std::shared_ptr<RemoteParticipantImpl> participant;
        if (const auto ndx = participantIndexBySid(sid)) {
            participant = _participants->at(ndx.value());
            _participants->erase(_participants->begin() + ndx.value());
        }
        dispose(participant);
    }
}

void RemoteParticipants::clearParticipants()
{
    for (const auto& participant : _participants.take()) {
        dispose(participant);
    }
}

void RemoteParticipants::dispose(const std::shared_ptr<RemoteParticipantImpl>& participant)
{
    if (participant) {
        for (const auto& trackInfo : participant->info()._tracks) {
            _nonBindedReceivers->take(trackInfo._sid);
        }
        participant->reset();
        if (_listener) {
            _listener->onParticipantRemoved(participant->sid());
        }
    }
}

std::optional<size_t> RemoteParticipants::participantIndexBySid(const std::string& sid) const
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
