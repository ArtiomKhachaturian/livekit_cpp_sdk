#include "participant.h"
#include <livekit/rtc/media/AudioTrack.h>
#include <livekit/rtc/media/VideoTrack.h>

Participant::Participant(QObject *parent)
    : QObject{parent}
{
}

Participant::~Participant()
{
    clearAudioTracks();
    clearVideoTracks();
}

void Participant::notifyThatSpeakerInfoChanged(float level, bool active)
{
    emit speakerInfoChanged(level, active);
}

AudioTrack* Participant::audioTrack(const QString& id) const
{
    return track(id, _audioTracks);
}

AudioTrack* Participant::audioTrack(qsizetype index) const
{
    if (index >= 0 && index < _audioTracks.size()) {
        return _audioTracks.at(index);
    }
    return nullptr;
}

VideoTrack* Participant::videoTrack(const QString& id) const
{
    return track(id, _videoTracks);
}

VideoTrack* Participant::videoTrack(qsizetype index) const
{
    if (index >= 0 && index < _videoTracks.size()) {
        return _videoTracks.at(index);
    }
    return nullptr;
}

void Participant::clearAudioTracks()
{
    clearTracks(_audioTracks);
}

void Participant::clearVideoTracks()
{
    clearTracks(_videoTracks);
}

QString Participant::addAudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack)
{
    if (const auto track = addAudio<>(sdkTrack)) {
        return track->id();
    }
    return {};
}

QString Participant::addVideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack)
{
    if (const auto track = addVideo<>(sdkTrack)) {
        return track->id();
    }
    return {};
}

bool Participant::removeAudioTrack(const QString& id)
{
    return remove(id, _audioTracks);
}

bool Participant::removeVideoTrack(const QString& id)
{
    return remove(id, _videoTracks);
}

void Participant::muteAudioTrack(const QString& id, bool mute)
{
    muteTrack(id, mute, _audioTracks);
}

void Participant::muteVideoTrack(const QString& id, bool mute)
{
    muteTrack(id, mute, _videoTracks);
}

void Participant::queryVideoStats()
{
    for (qsizetype i = 0; i < _videoTracks.count(); ++i) {
        _videoTracks[i]->queryStats();
    }
}

template <class TTrack>
bool Participant::remove(const QString& id, Tracks<TTrack>& tracks)
{
    if (!id.isEmpty()) {
        for (qsizetype i = 0; i < tracks.count(); ++i) {
            if (tracks.at(i)->id() == id) {
                destroy(tracks.at(i));
                tracks.removeAt(i);
                return true;
            }
        }
    }
    return false;
}

template <class TTrack>
void Participant::clearTracks(Tracks<TTrack>& tracks)
{
    for (qsizetype i = 0; i < tracks.count(); ++i) {
        destroy(tracks.at(i));
    }
    tracks.clear();
}

template <class TTrack>
void Participant::destroy(TTrack* track)
{
    if (track) {
        if constexpr (std::is_base_of_v<VideoTrack, TTrack>) {
            emit videoTrackRemoved(track->id());
            emit videoTracksCountChanged();
        }
        else if constexpr (std::is_base_of_v<AudioTrack, TTrack>) {
            emit audioTrackRemoved(track->id());
            emit audioTracksCountChanged();
        }
        else {
            static_assert(false, "incorrect media track type");
        }
        delete track;
    }
}

template <class TTrack>
TTrack* Participant::track(const QString& id, const Tracks<TTrack>& tracks)
{
    if (!id.isEmpty()) {
        for (qsizetype i = 0; i < tracks.count(); ++i) {
            if (tracks.at(i)->id() == id) {
                return tracks.at(i);
            }
        }
    }
    return nullptr;
}

template <class TTrack>
void Participant::muteTrack(const QString& id, bool mute, const Tracks<TTrack>& tracks)
{
    if (const auto track = Participant::track<TTrack>(id, tracks)) {
        track->setMuted(mute);
    }
}
