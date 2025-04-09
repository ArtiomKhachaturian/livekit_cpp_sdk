#ifndef REMOTEPARTICIPANT_H
#define REMOTEPARTICIPANT_H
#include <QQmlEngine>
#include "participant.h"

class RemoteParticipant : public Participant
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit RemoteParticipant(QObject *parent = nullptr);
};

#endif // REMOTEPARTICIPANT_H
