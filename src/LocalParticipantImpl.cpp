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
#include "LocalParticipantImpl.h"
#include "Blob.h"
#include "CameraVideoTrack.h"
#include "CameraVideoSource.h"
#include "CameraManager.h"
#include "DataChannel.h"
#include "Utils.h"
#include "PeerConnectionFactory.h"
#include "rtc/ParticipantInfo.h"

namespace {

using namespace LiveKitCpp;

inline webrtc::scoped_refptr<webrtc::AudioTrackInterface>
    createMic(PeerConnectionFactory* pcf) {
    if (pcf) {
        cricket::AudioOptions options; // TODO: should be a part of room config
        if (const auto source = pcf->CreateAudioSource(options)) {
            return pcf->CreateAudioTrack(makeUuid(), source.get());
        }
    }
    return {};
}

inline webrtc::scoped_refptr<CameraVideoTrack>
    createCamera(PeerConnectionFactory* pcf,
                 const std::shared_ptr<Bricks::Logger>& logger) {
    if (pcf && CameraManager::available()) {
        auto source = webrtc::make_ref_counted<CameraVideoSource>(pcf->signalingThread(),
                                                                  logger);
        return webrtc::make_ref_counted<CameraVideoTrack>(makeUuid(),
                                                          std::move(source),
                                                          logger);
    }
    return {};
}

}

namespace LiveKitCpp
{

LocalParticipantImpl::LocalParticipantImpl(TrackManager* manager,
                                           PeerConnectionFactory* pcf,
                                           const std::shared_ptr<Bricks::Logger>& logger)
    : _microphone(createMic(pcf), manager, true, logger)
    , _camera(createCamera(pcf, logger), manager, logger)
{
}

LocalParticipantImpl::~LocalParticipantImpl()
{
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
    LocalParticipantImpl::tracks() const
{
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> tracks;
    tracks.reserve(2U);
    if (const auto track = _microphone.rtcTrack()) {
        tracks.push_back(track);
    }
    if (const auto track = _camera.rtcTrack()) {
        tracks.push_back(track);
    }
    return tracks;
}

LocalTrack* LocalParticipantImpl::track(const std::string& id, bool cid)
{
    if (!id.empty()) {
        if (id == (cid ? _microphone.cid() : _microphone.sid())) {
            return &_microphone;
        }
        if (id == (cid ? _camera.cid() : _camera.sid())) {
            return &_camera;
        }
    }
    return nullptr;
}

LocalTrack* LocalParticipantImpl::track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    if (sender) {
        return track(sender->id(), true);
    }
    return nullptr;
}

void LocalParticipantImpl::setInfo(const ParticipantInfo& info)
{
    bool changed = false;
    if (exchangeVal(info._sid, _sid)) {
        changed = true;
    }
    if (exchangeVal(info._identity, _identity)) {
        changed = true;
    }
    if (exchangeVal(info._name, _name)) {
        changed = true;
    }
    if (exchangeVal(info._metadata, _metadata)) {
        changed = true;
    }
    if (exchangeVal(info._kind, _kind)) {
        changed = true;
    }
    if (changed) {
        fireOnChanged();
    }
}

} // namespace LiveKitCpp
#endif
