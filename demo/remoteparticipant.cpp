#include "remoteparticipant.h"
#include <livekit/RemoteParticipant.h>

RemoteParticipant::RemoteParticipant(QObject* parent)
    : Participant{parent}
{
}

RemoteParticipant::RemoteParticipant(const std::shared_ptr<LiveKitCpp::RemoteParticipant>& participant,
                                     QObject* parent)
    : Participant(parent)
    , _participant(participant)
{
    if (participant) {
        for (size_t i = 0U; i < participant->audioTracksCount(); ++i) {
            addAudioTrack(participant->audioTrack(i));
        }
        for (size_t i = 0U; i < participant->videoTracksCount(); ++i) {
            addVideoTrack(participant->videoTrack(i));
        }
        participant->setListener(this);
    }
}

RemoteParticipant::~RemoteParticipant()
{
    if (_participant) {
        _participant->setListener(nullptr);
    }
}

QString RemoteParticipant::sid() const
{
    if (_participant) {
        return QString::fromStdString(_participant->sid());
    }
    return {};
}

QString RemoteParticipant::identity() const
{
    if (_participant) {
        return QString::fromStdString(_participant->identity());
    }
    return {};
}

QString RemoteParticipant::name() const
{
    if (_participant) {
        return QString::fromStdString(_participant->name());
    }
    return {};
}

void RemoteParticipant::onChanged(const LiveKitCpp::Participant* sender)
{
    if (sender && sender == _participant.get()) {
        emit sidChanged();
        emit identityChanged();
        emit nameChanged();
    }
}

void RemoteParticipant::addMediaTrack(bool audio, const QString& sid)
{
    if (_participant) {
        if (audio) {
            addAudioTrack(_participant->audioTrack(sid.toStdString()));
        }
        else {
            addVideoTrack(_participant->videoTrack(sid.toStdString()));
        }
    }
}

void RemoteParticipant::removeMediaTrack(bool audio, const QString& sid)
{
    if (audio) {
        removeAudioTrack(sid);
    }
    else {
        removeVideoTrack(sid);
    }
}

void RemoteParticipant::onRemoteTrackAdded(const LiveKitCpp::RemoteParticipant* sender,
                                           LiveKitCpp::TrackType type,
                                           LiveKitCpp::EncryptionType,
                                           const std::string& sid)
{
    if (_participant && _participant.get() == sender) {
        switch (type) {
            case LiveKitCpp::TrackType::Audio:
                QMetaObject::invokeMethod(this, &RemoteParticipant::addMediaTrack,
                                          true, QString::fromStdString(sid));
                break;
            case LiveKitCpp::TrackType::Video:
                QMetaObject::invokeMethod(this, &RemoteParticipant::addMediaTrack,
                                          false, QString::fromStdString(sid));
                break;
            case LiveKitCpp::TrackType::Data:
                break;
        }
    }
}

void RemoteParticipant::onRemoteTrackRemoved(const LiveKitCpp::RemoteParticipant* sender,
                                             LiveKitCpp::TrackType type,
                                             const std::string& sid)
{
    if (_participant && _participant.get() == sender) {
        switch (type) {
            case LiveKitCpp::TrackType::Audio:
                QMetaObject::invokeMethod(this, &RemoteParticipant::removeMediaTrack,
                                          true, QString::fromStdString(sid));
                break;
            case LiveKitCpp::TrackType::Video:
                QMetaObject::invokeMethod(this, &RemoteParticipant::removeMediaTrack,
                                          false, QString::fromStdString(sid));
                break;
            case LiveKitCpp::TrackType::Data:
                break;
        }
    }
}
