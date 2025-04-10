#ifndef REMOTEPARTICIPANT_H
#define REMOTEPARTICIPANT_H
#include "participant.h"
#include <livekit/RemoteParticipantListener.h>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class RemoteParticipant;
}

class RemoteParticipant : public Participant,
                          private LiveKitCpp::RemoteParticipantListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RemoteParticipant)
public:
    explicit RemoteParticipant(QObject* parent = nullptr);
    RemoteParticipant(const std::shared_ptr<LiveKitCpp::RemoteParticipant>& participant,
                      QObject* parent = nullptr);
    ~RemoteParticipant();
    // overrides of Participant
    QString sid() const final;
    QString identity() const final;
    QString name() const final;
private slots:
    void addMediaTrack(bool audio, const QString& sid);
    void removeMediaTrack(bool audio, const QString& sid);
private:
    // impl. of LiveKitCpp::RemoteParticipantListener
    void onChanged(const LiveKitCpp::Participant* sender) final;
    void onRemoteTrackAdded(const LiveKitCpp::RemoteParticipant* sender,
                            LiveKitCpp::TrackType type,
                            LiveKitCpp::EncryptionType,
                            const std::string& sid) final;
    void onRemoteTrackRemoved(const LiveKitCpp::RemoteParticipant* sender,
                              LiveKitCpp::TrackType type,
                              const std::string& sid) final;
private:
    const std::shared_ptr<LiveKitCpp::RemoteParticipant> _participant;
};

Q_DECLARE_METATYPE(RemoteParticipant*)

#endif // REMOTEPARTICIPANT_H
