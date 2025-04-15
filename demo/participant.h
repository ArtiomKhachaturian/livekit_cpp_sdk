#ifndef PARTICIPANT_H
#define PARTICIPANT_H
#include "audiotrack.h"
#include "videotrack.h"
#include <livekit/rtc/media/Track.h>
#include <QObject>
#include <QQmlEngine>
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
    Q_PROPERTY(qsizetype audioTracksCount READ audioTracksCount NOTIFY audioTracksCountChanged)
    Q_PROPERTY(qsizetype videoTracksCount READ videoTracksCount NOTIFY videoTracksCountChanged)
    template <class TTrack> using Tracks = QList<TTrack*>;
public:
    explicit Participant(QObject *parent = nullptr);
    ~Participant() override;
    void notifyThatSpeakerInfoChanged(float level, bool active);
    virtual QString sid() const { return {}; }
    virtual QString identity() const { return {}; }
    virtual QString name() const { return {}; }
    qsizetype audioTracksCount() const noexcept { return _audioTracks.size(); }
    qsizetype videoTracksCount() const noexcept { return _videoTracks.size(); }
    Q_INVOKABLE AudioTrack* audioTrack(const QString& id) const;
    Q_INVOKABLE AudioTrack* audioTrack(qsizetype index) const;
    Q_INVOKABLE VideoTrack* videoTrack(const QString& id) const;
    Q_INVOKABLE VideoTrack* videoTrack(qsizetype index) const;
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
    void audioTracksCountChanged();
    void videoTracksCountChanged();
    void speakerInfoChanged(float level, bool active);
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
            track = new TTrack(sdkTrack, this);
            tracks.append(track);
        }
        else {
            Q_ASSERT(false);
        }
        if (track) {
            if constexpr (std::is_base_of_v<VideoTrack, TTrack>) {
                emit videoTrackAdded(id);
                emit videoTracksCountChanged();
            }
            else if constexpr (std::is_base_of_v<AudioTrack, TTrack>) {
                emit audioTrackAdded(id);
                emit audioTracksCountChanged();
            }
            else {
                static_assert(false, "incorrect media track type");
            }
        }
    }
    return track;
}

#endif // PARTICIPANT_H
