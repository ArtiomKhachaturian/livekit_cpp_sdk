#include "participant.h"
#include <livekit/media/AudioTrack.h>
#include <livekit/media/VideoTrack.h>

Participant::Participant(QObject *parent)
    : QObject{parent}
{
}

Participant::~Participant()
{
    clearAudioTracks();
    clearVideoTracks();
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

QList<std::shared_ptr<LiveKitCpp::AudioTrack>> Participant::clearAudioTracks()
{
    return clearTracks<LiveKitCpp::AudioTrack>(_audioTracks);
}

QList<std::shared_ptr<LiveKitCpp::VideoTrack>> Participant::clearVideoTracks()
{
    return clearTracks<LiveKitCpp::VideoTrack>(_videoTracks);
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

std::shared_ptr<LiveKitCpp::AudioTrack> Participant::removeAudioTrack(const QString& id)
{
    return remove<LiveKitCpp::AudioTrack>(id, _audioTracks);
}

std::shared_ptr<LiveKitCpp::VideoTrack> Participant::removeVideoTrack(const QString& id)
{
    return remove<LiveKitCpp::VideoTrack>(id, _videoTracks);
}

void Participant::muteAudioTrack(const QString& id, bool mute)
{
    muteTrack(id, mute, _audioTracks);
}

void Participant::muteVideoTrack(const QString& id, bool mute)
{
    muteTrack(id, mute, _videoTracks);
}

template <class TSdkTrack, class TTrack>
std::shared_ptr<TSdkTrack> Participant::remove(const QString& id, Tracks<TTrack>& tracks)
{
    std::shared_ptr<TSdkTrack> track;
    if (!id.isEmpty()) {
        for (qsizetype i = 0; i < tracks.count(); ++i) {
            if (tracks.at(i)->id() == id) {
                track = takeSdkTrackAndDestroy<TSdkTrack>(tracks.at(i));
                tracks.removeAt(i);
            }
        }
    }
    return track;
}

template <class TSdkTrack, class TTrack>
QList<std::shared_ptr<TSdkTrack>> Participant::clearTracks(Tracks<TTrack>& tracks)
{
    QList<std::shared_ptr<TSdkTrack>> sdkTracks;
    sdkTracks.reserve(tracks.size());
    for (qsizetype i = 0; i < tracks.count(); ++i) {
        sdkTracks.push_back(takeSdkTrackAndDestroy<TSdkTrack>(tracks.at(i)));
    }
    return sdkTracks;
}

template <class TSdkTrack, class TTrack>
std::shared_ptr<TSdkTrack> Participant::takeSdkTrackAndDestroy(TTrack* track)
{
    std::shared_ptr<TSdkTrack> sdkTrack;
    if (track) {
        sdkTrack = track->takeSdkTrack();
        if constexpr (std::is_base_of_v<VideoTrack, TTrack>) {
            emit videoTrackRemoved(QString::fromStdString(sdkTrack->id()));
            emit videoTracksCountChanged();
        }
        else if constexpr (std::is_base_of_v<AudioTrack, TTrack>) {
            emit audioTrackRemoved(QString::fromStdString(sdkTrack->id()));
            emit audioTracksCountChanged();
        }
        else {
            static_assert(false, "incorrect media track type");
        }
        delete track;
    }
    return sdkTrack;
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
