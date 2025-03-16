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
        for (const auto& info : infos) {
            auto participant = std::make_shared<RemoteParticipantImpl>();
            participant->setInfo(info);
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
    if (receiver) {
        /*TrackManager* manager = this;
        auto sid = RemoteTrack::sid(transceiver->receiver());
        if (!sid.empty()) {
            auto track = std::make_unique<RemoteTrack>(manager, transceiver->receiver());
            LOCK_WRITE_SAFE_OBJ(_remoteTracks);
            _remoteTracks->insert(std::make_pair(std::move(sid), std::move(track)));
        }
        else {
            logWarning("empty ID of received transceiver");
        }*/
    }
    return false;
}

bool RemoteParticipantsImpl::removeMedia(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver)
{
    if (receiver) {
        
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

std::string_view RemoteParticipantsImpl::logCategory() const
{
    static const std::string_view category("remote_participants");
    return category;
}

} // namespace LiveKitCpp
