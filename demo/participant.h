#ifndef PARTICIPANT_H
#define PARTICIPANT_H
#include "lockable.h"
#include "audiotrack.h"
#include "videotrack.h"
#include <livekit/media/Track.h>
#include <QObject>
#include <QQmlEngine>
#include <QHash>
#include <type_traits>

namespace LiveKitCpp {
class AudioTrack;
class VideoTrack;
}

class Participant : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Participant)
    Q_PROPERTY(QString sid READ sid NOTIFY sidChanged)
    Q_PROPERTY(QString identity READ identity NOTIFY identityChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    template <class TTrack> using Tracks = Lockable<QHash<QString, TTrack*>>;
public:
    explicit Participant(QObject *parent = nullptr);
    ~Participant() override;
    virtual QString sid() const { return {}; }
    virtual QString identity() const { return {}; }
    virtual QString name() const { return {}; }
    Q_INVOKABLE AudioTrack* audioTrack(const QString& id) const;
    Q_INVOKABLE VideoTrack* videoTrack(const QString& id) const;
    QList<std::shared_ptr<LiveKitCpp::AudioTrack>> clearAudioTracks();
    QList<std::shared_ptr<LiveKitCpp::VideoTrack>> clearVideoTracks();
    // return ID of SDK track
    QString addAudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack);
    QString addVideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack);
    // return SDK track
    std::shared_ptr<LiveKitCpp::AudioTrack> removeAudioTrack(const QString& id);
    std::shared_ptr<LiveKitCpp::VideoTrack> removeVideoTrack(const QString& id);
public slots:
    void muteAudioTrack(const QString& id, bool mute = true);
    void muteVideoTrack(const QString& id, bool mute = true);
    void unmuteAudioTrack(const QString& id) { muteAudioTrack(id, false); }
    void unmuteVideoTrack(const QString& id) { muteVideoTrack(id, false); }
signals:
    void audioTrackAdded(const QString& id);
    void videoTrackAdded(const QString& id);
    void audioTrackRemoved(const QString& id);
    void videoTrackRemoved(const QString& id);
    void sidChanged();
    void identityChanged();
    void nameChanged();
protected:
    template <class TTrack = VideoTrack, class TSdkTrack = LiveKitCpp::VideoTrack>
    TTrack* addVideo(const std::shared_ptr<TSdkTrack>& sdkTrack);
    template <class TTrack = AudioTrack, class TSdkTrack = LiveKitCpp::AudioTrack>
    TTrack* addAudio(const std::shared_ptr<TSdkTrack>& sdkTrack);
private:
    template <class TTrack, class TSdkTrack, class TContainerTrack = TTrack>
    TTrack* add(const std::shared_ptr<TSdkTrack>& sdkTrack, Tracks<TContainerTrack>& tracks);
    template <class TSdkTrack, class TTrack>
    std::shared_ptr<TSdkTrack> remove(const QString& id, Tracks<TTrack>& tracks);
    template <class TSdkTrack, class TTrack>
    QList<std::shared_ptr<TSdkTrack>> clearTracks(Tracks<TTrack>& tracks);
    template <class TSdkTrack, class TTrack>
    std::shared_ptr<TSdkTrack> takeSdkTrackAndDestroy(TTrack* track);
    template <class TTrack>
    static TTrack* track(const QString& id, const Tracks<TTrack>& tracks);
    template <class TTrack>
    static void muteTrack(const QString& id, bool mute, const Tracks<TTrack>& tracks);
private:
    Tracks<AudioTrack> _audioTracks;
    Tracks<VideoTrack> _videoTracks;
};

template <class TTrack, class TSdkTrack>
inline TTrack* Participant::addVideo(const std::shared_ptr<TSdkTrack>& sdkTrack)
{
    return add<TTrack, TSdkTrack>(sdkTrack, _videoTracks);
}

template <class TTrack, class TSdkTrack>
inline TTrack* Participant::addAudio(const std::shared_ptr<TSdkTrack>& sdkTrack)
{
    return add<TTrack, TSdkTrack>(sdkTrack, _audioTracks);
}

template <class TTrack, class TSdkTrack, class TContainerTrack>
inline TTrack* Participant::add(const std::shared_ptr<TSdkTrack>& sdkTrack,
                                Tracks<TContainerTrack>& tracks)
{
    TTrack* track = nullptr;
    if (sdkTrack) {
        const auto id = QString::fromStdString(sdkTrack->id());
        if (!id.isEmpty()) {
            const QWriteLocker locker(&tracks._lock);
            Q_ASSERT(!tracks._val.contains(id));
            track = new TTrack(sdkTrack, this);
            tracks._val[id] = track;
        }
        else {
            Q_ASSERT(false);
        }
        if (track) {
            if constexpr (std::is_base_of_v<VideoTrack, TTrack>) {
                emit videoTrackAdded(id);
            }
            else if constexpr (std::is_base_of_v<AudioTrack, TTrack>) {
                emit audioTrackAdded(id);
            }
            else {
                static_assert(false, "incorrect media track type");
            }
        }
    }
    return track;
}

#endif // PARTICIPANT_H
