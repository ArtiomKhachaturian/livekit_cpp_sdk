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
#include "LocalTrack.h"
#include "LocalTrackManager.h"
#include "Utils.h"
#include "rtc/AddTrackRequest.h"
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>

namespace LiveKitCpp
{

LocalTrack::LocalTrack(std::string name, LocalTrackManager* manager)
    : _cid(makeUuid())
    , _name(std::move(name))
    , _manager(manager)
{
}

LocalTrack::~LocalTrack()
{
    resetTrackSender(true);
}

void LocalTrack::mute(bool mute)
{
    if (mute != _muted.exchange(mute)) {
        LOCK_WRITE_SAFE_OBJ(_sender);
        if (mute) {
            _requested = false;
            muteSender(true, true);
        }
        else {
            if (_sender.constRef()) {
                muteSender(true, false);
            }
            else if (!_requested && _manager) {
                if (auto track = createMediaTrack(_cid)) {
                    _requested = _manager->add(std::move(track));
                }
            }
        }
    }
}

SetSenderResult LocalTrack::setTrackSender(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (_requested && sender) {
        if (accept(sender)) {
            _sender = sender;
            _requested = false;
            muteSender(muted(), false);
            return SetSenderResult::Accepted;
        }
        return SetSenderResult::NotMatchedToRequest;
    }
    return SetSenderResult::Rejected;
}

bool LocalTrack::resetTrackSender(bool full)
{
    bool ok = false;
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (auto sender = _sender.take()) {
        if (full && _manager) {
            ok = _manager->remove(std::move(sender));
        }
        else {
            ok = true;
        }
    }
    else {
        _requested = false;
        ok = true;
    }
    return ok;
}

bool LocalTrack::fillRequest(AddTrackRequest& request) const
{
    LOCK_READ_SAFE_OBJ(_sender);
    if (_sender.constRef()) {
        switch (mediaType()) {
            case cricket::MEDIA_TYPE_AUDIO:
                request._type = TrackType::Audio;
                break;
            case cricket::MEDIA_TYPE_VIDEO:
                request._type = TrackType::Video;
                break;
            case cricket::MEDIA_TYPE_DATA:
                request._type = TrackType::Data;
                break;
            default:
                break;
        }
        request._cid = _cid;
        request._name = _name;
        request._muted = _muted;
        request._sid = _sid;
        return true;
    }
    return false;
}

void LocalTrack::muteSender(bool mute, bool notify = true) const
{
    if (const auto& sender = _sender.constRef()) {
        if (const auto track = sender->track()) {
            const auto wasEnabled = track->enabled();
            if (wasEnabled == mute) {
                track->set_enabled(!mute);
                if (notify && _manager) {
                    _manager->notifyAboutMuteChanges(_sid, mute);
                }
            }
        }
    }
}

bool LocalTrack::accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender && sender->media_type() == mediaType()) {
        if (const auto track = sender->track()) {
            return accept(track->id());
        }
    }
    return false;
}

} // namespace LiveKitCpp
