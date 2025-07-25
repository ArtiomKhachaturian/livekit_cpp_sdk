#include "audiotrack.h"
#include <livekit/rtc/media/LocalAudioTrack.h>
#include <livekit/rtc/media/RemoteAudioTrack.h>

AudioTrack::AudioTrack(QObject* parent)
    : QObject(parent)
{
}

AudioTrack::AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack,
                       QObject *parent)
    : QObject(parent)
    , _sdkTrack(sdkTrack)
{
    if (_sdkTrack) {
        _sdkTrack->addListener(this);
    }
}

AudioTrack::~AudioTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeListener(this);
    }
}

void AudioTrack::setVolume(qreal volume)
{
    if (_sdkTrack) {
        _sdkTrack->setVolume(volume);
    }
}

QString AudioTrack::id() const
{
    if (_sdkTrack) {
        return QString::fromStdString(_sdkTrack->id());
    }
    return {};
}

bool AudioTrack::isRemote() const
{
    return _sdkTrack && _sdkTrack->remote();
}

bool AudioTrack::isMuted() const
{
    return _sdkTrack && _sdkTrack->muted();
}

bool AudioTrack::isSecure() const
{
    return _sdkTrack && LiveKitCpp::EncryptionType::None != _sdkTrack->encryption();
}

bool AudioTrack::isFirstPacketSentOrReceived() const
{
    if (const auto sdkTrack = std::dynamic_pointer_cast<LiveKitCpp::LocalAudioTrack>(_sdkTrack)) {
        return sdkTrack->firstPacketSent();
    }
    if (const auto sdkTrack = std::dynamic_pointer_cast<LiveKitCpp::RemoteAudioTrack>(_sdkTrack)) {
        return sdkTrack->firstPacketReceived();
    }
    return false;
}

void AudioTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}
