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

VideoTrack* Participant::videoTrack(const QString& id) const
{
    return track(id, _videoTracks);
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
        const auto it = tracks.find(id);
        if (it != tracks.end()) {
            track = takeSdkTrackAndDestroy<TSdkTrack>(it.value());
            tracks.erase(it);
        }
    }
    return track;
}

template <class TSdkTrack, class TTrack>
QList<std::shared_ptr<TSdkTrack>> Participant::clearTracks(Tracks<TTrack>& tracks)
{
    QList<std::shared_ptr<TSdkTrack>> sdkTracks;
    sdkTracks.reserve(tracks.size());
    for (auto it = tracks.begin(); it != tracks.end(); ++it) {
        sdkTracks.push_back(takeSdkTrackAndDestroy<TSdkTrack>(it.value()));
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
        }
        else if constexpr (std::is_base_of_v<AudioTrack, TTrack>) {
            emit audioTrackRemoved(QString::fromStdString(sdkTrack->id()));
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
        const auto it = tracks.constFind(id);
        if (it != tracks.constEnd()) {
            return it.value();
        }
    }
    return nullptr;
}

template <class TTrack>
void Participant::muteTrack(const QString& id, bool mute, const Tracks<TTrack>& tracks)
{
    if (!id.isEmpty()) {
        const auto it = tracks.constFind(id);
        if (it != tracks.constEnd()) {
            it.value()->setMuted(mute);
        }
    }
}
