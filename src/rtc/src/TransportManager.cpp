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
#include "TransportManager.h"
#include "TransportManagerImpl.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

TransportManager::TransportManager(bool subscriberPrimary, bool fastPublish,
                                   int32_t pingTimeout, int32_t pingInterval,
                                   uint64_t negotiationDelay,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                   const std::weak_ptr<TrackManager>& trackManager,
                                   const std::string& identity,
                                   const std::string& prefferedAudioEncoder,
                                   const std::string& prefferedVideoEncoder,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : RtcObject<TransportManagerImpl>(subscriberPrimary, fastPublish, pingTimeout,
                                      pingInterval, negotiationDelay, pcf, conf, trackManager,
                                      identity, prefferedAudioEncoder, prefferedVideoEncoder, logger)
{
}

TransportManager::~TransportManager()
{
    close();
}

bool TransportManager::valid() const noexcept
{
    const auto impl = loadImpl();
    return impl && impl->valid();
}

webrtc::PeerConnectionInterface::PeerConnectionState TransportManager::state() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->state();
    }
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
}

bool TransportManager::closed() const noexcept
{
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed == state();
}

void TransportManager::negotiate(bool force)
{
    if (const auto impl = loadImpl()) {
        impl->negotiate(force);
    }
}

void TransportManager::startPing()
{
    if (const auto impl = loadImpl()) {
        impl->startPing();
    }
}

void TransportManager::stopPing()
{
    if (const auto impl = loadImpl()) {
        impl->stopPing();
    }
}

void TransportManager::notifyThatPongReceived()
{
    if (const auto impl = loadImpl()) {
        impl->notifyThatPongReceived();
    }
}

bool TransportManager::setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        const auto impl = loadImpl();
        return impl && impl->setRemoteOffer(std::move(desc));
    }
    return false;
}

bool TransportManager::setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        const auto impl = loadImpl();
        return impl && impl->setRemoteAnswer(std::move(desc));
    }
    return false;
}

bool TransportManager::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    const auto impl = loadImpl();
    return impl && impl->setConfiguration(config);
}

void TransportManager::addTrack(std::shared_ptr<AudioDeviceImpl> device, EncryptionType encryption)
{
    if (device) {
        if (const auto impl = loadImpl()) {
            impl->addTrack(std::move(device), encryption);
        }
    }
}

void TransportManager::addTrack(std::shared_ptr<LocalVideoDeviceImpl> device, EncryptionType encryption)
{
    if (device) {
        if (const auto impl = loadImpl()) {
            impl->addTrack(std::move(device), encryption);
        }
    }
}

bool TransportManager::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (track) {
        const auto impl = loadImpl();
        return impl && impl->removeTrack(track);
    }
    return false;
}

void TransportManager::addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    if (candidate) {
        if (const auto impl = loadImpl()) {
            impl->addIceCandidate(target, std::move(candidate));
        }
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback) {
        if (const auto impl = loadImpl()) {
            impl->queryStats(callback);
        }
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                  const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (receiver && callback) {
        if (const auto impl = loadImpl()) {
            impl->queryReceiverStats(receiver, callback);
        }
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                                  const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (sender && callback) {
        if (const auto impl = loadImpl()) {
            impl->querySenderStats(sender, callback);
        }
    }
}

void TransportManager::setAudioPlayout(bool playout)
{
    if (const auto impl = loadImpl()) {
        impl->setAudioPlayout(playout);
    }
}

void TransportManager::setAudioRecording(bool recording)
{
    if (const auto impl = loadImpl()) {
        impl->setAudioRecording(recording);
    }
}

void TransportManager::close()
{
    if (auto impl = dispose()) {
        impl->close();
    }
}

void TransportManager::setListener(TransportManagerListener* listener)
{
    if (const auto impl = loadImpl()) {
        impl->setListener(listener);
    }
}

} // namespace LiveKitCpp
